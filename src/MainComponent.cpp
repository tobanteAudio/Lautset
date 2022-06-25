#include "MainComponent.hpp"

#include <juce_dsp/juce_dsp.h>

namespace ta
{

auto analyseFile(BufferWithSampleRate buffer, Milliseconds<float> windowLength) -> std::vector<LevelWindow>
{
    auto const windowSampleCount = static_cast<std::size_t>(toSampleCount(windowLength, buffer.sampleRate));
    auto const numWindows        = static_cast<std::size_t>(buffer.buffer.getNumSamples()) / windowSampleCount;

    auto rmsWindows = std::vector<LevelWindow>{};
    rmsWindows.reserve(numWindows);

    for (auto i{0U}; i < numWindows; ++i)
    {
        auto const windowStart = i * windowSampleCount;
        auto const peak        = buffer.buffer.getMagnitude(0, windowStart, windowSampleCount);
        auto const rms         = buffer.buffer.getRMSLevel(0, windowStart, windowSampleCount);
        rmsWindows.push_back(LevelWindow{peak, rms});
    }

    return rmsWindows;
}

}  // namespace ta

auto positionForGain(float gain, float top, float bottom) -> float
{
    static constexpr auto minDB = -30.0f;
    static constexpr auto maxDB = 0.1f;
    return juce::jmap(juce::Decibels::gainToDecibels(gain, minDB), minDB, maxDB, bottom, top);
}

template<typename ContainerAccess>
auto containerToPath(std::size_t size, juce::Rectangle<int> area, ContainerAccess access) -> juce::Path
{
    if (size == 0U) { return {}; }

    juce::Path path{};
    path.preallocateSpace(static_cast<int>(size));

    auto const firstY = positionForGain(access(0U), area.getY(), area.getBottom());
    path.startNewSubPath(juce::Point<float>(0.0f, firstY));

    for (auto i{1U}; i < size; ++i)
    {
        auto const x = static_cast<float>(i) / size * area.getWidth();
        auto const y = positionForGain(access(i), area.getY(), area.getBottom());
        path.lineTo(juce::Point<float>(x, y));
    }

    return path;
}

auto rmsWindowsToPath(std::vector<ta::LevelWindow> const& windows, juce::Rectangle<int> area) -> juce::Path
{
    return containerToPath(windows.size(), area, [&windows](std::size_t i) { return windows[i].rms; });
}

auto applyBallisticFilter(ta::BufferWithSampleRate const& buf, ta::Milliseconds<double> env) -> juce::AudioBuffer<float>
{
    auto doubleInput = juce::AudioBuffer<double>{};
    doubleInput.makeCopyOf(buf.buffer);

    auto filter = juce::dsp::BallisticsFilter<double>{};
    filter.setLevelCalculationType(juce::dsp::BallisticsFilter<double>::LevelCalculationType::RMS);
    filter.setAttackTime(env.count());
    filter.setReleaseTime(env.count());
    filter.prepare({
        buf.sampleRate,
        static_cast<uint32_t>(buf.buffer.getNumChannels()),
        static_cast<uint32_t>(buf.buffer.getNumSamples()),
    });

    auto filtered = juce::AudioBuffer<double>{buf.buffer.getNumChannels(), buf.buffer.getNumSamples()};
    auto inBlock  = juce::dsp::AudioBlock<double const>{doubleInput};
    auto outBlock = juce::dsp::AudioBlock<double>{filtered};
    filter.process(juce::dsp::ProcessContextNonReplacing<double>{inBlock, outBlock});

    auto floatOutput = juce::AudioBuffer<float>{};
    floatOutput.makeCopyOf(filtered);

    return floatOutput;
}

MainComponent::MainComponent()
{
    addAndMakeVisible(_loadFile);
    addAndMakeVisible(_analyze);
    addAndMakeVisible(_rmsWindowLength);

    _rmsWindowLength.setRange(juce::Range<double>{1'000.0, 45'000.0}, 100.0);
    _rmsWindowLength.setValue(5'000.0);

    _loadFile.onClick = [this]
    {
        auto path = juce::File{"/home/tobante/Downloads/3 deck action volume adjust mp3.mp3"};
        // auto path = juce::File{"/home/tobante/Music/Loops/Drums.wav"};
        loadFile(path);
    };

    _analyze.onClick = [this] { analyseAudio(); };

    _formatManager.registerBasicFormats();
    setSize(600, 400);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    juce::ScopedLock lock{_mutex};

    g.setColour(juce::Colours::white);
    g.fillRect(_drawArea);

    if (_rmsWindows.empty()) { return; }

    auto rmsPath = rmsWindowsToPath(_rmsWindows, _drawArea);
    g.setColour(juce::Colours::black);
    g.strokePath(rmsPath, juce::PathStrokeType(1.0));

    if (_rmsBins.empty()) { return; }

    auto max = *std::max_element(_rmsBins.begin(), _rmsBins.end());

    for (auto i = 0U; i < _rmsBins.size(); ++i)
    {
        auto bin = _rmsBins[i];
        if (bin == 0.0f) { continue; }
        auto const height = _drawArea.getHeight() * ((float)bin / (float)max);
        auto const x      = static_cast<float>(_rmsBins.size() - i) / _rmsBins.size() * _drawArea.getWidth();
        g.fillRect(juce::Rectangle<float>(x, 0, 8.0f, height).withBottomY(_drawArea.getBottom()));

        auto textArea = juce::Rectangle<int>((int)x, 0, 40, 40).withBottomY(_drawArea.getBottom());
        g.drawText(juce::String(i), textArea, juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    _loadFile.setBounds(area.removeFromTop(area.proportionOfHeight(0.06f)));
    _analyze.setBounds(area.removeFromTop(area.proportionOfHeight(0.06f)));
    _rmsWindowLength.setBounds(area.removeFromBottom(area.proportionOfHeight(0.05f)));
    _drawArea = area;
}

auto MainComponent::loadFile(juce::File const& file) -> void
{
    auto task = [this, path = file]
    {
        if (!path.existsAsFile()) { return; }

        auto audioBuffer = ta::resampleAudioBuffer(ta::loadAudioFileToBuffer(path, 0), 44'100.0 / 8.0);
        if (audioBuffer.buffer.getNumSamples() == 0) { return; }

        {
            juce::ScopedLock lock{_mutex};
            _audioBuffer = std::move(audioBuffer);
        }

        analyseAudio();

        juce::MessageManager::callAsync([this] { repaint(); });
    };

    _threadPool.addJob(task);
}

auto MainComponent::analyseAudio() -> void
{
    auto task = [this, windowSize = ta::Milliseconds<float>{_rmsWindowLength.getValue()}]
    {
        auto audioBuffer = ta::BufferWithSampleRate{};
        {
            juce::ScopedLock lock{_mutex};
            audioBuffer = _audioBuffer;
        }

        auto rmsWindows = ta::analyseFile(audioBuffer, windowSize);

        auto rmsBins = std::vector<int>{};
        rmsBins.resize(81U);
        for (auto const& window : rmsWindows)
        {
            auto dB = juce::Decibels::gainToDecibels(window.rms);
            if (dB <= -80.0f) { continue; }
            if (dB > 0.0f) { continue; }

            auto index = static_cast<std::size_t>(std::round(std::abs(dB)));
            rmsBins[index] += 1;
        }

        {
            juce::ScopedLock lock{_mutex};
            _rmsWindows = std::move(rmsWindows);
            _rmsBins    = std::move(rmsBins);
        }

        juce::MessageManager::callAsync([this] { repaint(); });
    };

    _threadPool.addJob(task);
}