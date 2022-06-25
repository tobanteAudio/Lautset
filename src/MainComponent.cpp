#include "MainComponent.hpp"

#include <juce_dsp/juce_dsp.h>

namespace ta
{

namespace
{

auto positionForGain(float gain, float top, float bottom) -> float
{
    static constexpr auto minDB = -30.0f;
    static constexpr auto maxDB = 0.1f;
    return juce::jmap(juce::Decibels::gainToDecibels(gain, minDB), minDB, maxDB, bottom, top);
}

template<typename ContainerAccess>
auto containerToPath(std::size_t size, juce::Rectangle<float> area, ContainerAccess access) -> juce::Path
{
    if (size == 0U) { return {}; }

    juce::Path path{};
    path.preallocateSpace(static_cast<int>(size));

    auto const firstY = positionForGain(access(0U), area.getY(), area.getBottom());
    path.startNewSubPath(juce::Point<float>(0.0f, firstY));

    for (auto i{1U}; i < size; ++i)
    {
        auto const x = static_cast<float>(i) / (float)size * area.getWidth();
        auto const y = positionForGain(access(i), area.getY(), area.getBottom());
        path.lineTo(juce::Point<float>(x, y));
    }

    return path;
}

auto rmsWindowsToPath(std::vector<LevelWindow> const& windows, juce::Rectangle<float> area) -> juce::Path
{
    return containerToPath(windows.size(), area, [&windows](std::size_t i) { return windows[i].rms; });
}

}  // namespace
}  // namespace ta

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
    g.fillRect(_rmsWindowsArea);
    g.fillRect(_rmsBinsArea);

    if (_analysis.rmsWindows.empty()) { return; }

    auto rmsPath = ta::rmsWindowsToPath(_analysis.rmsWindows, _rmsWindowsArea);
    g.setColour(juce::Colours::black);
    g.strokePath(rmsPath, juce::PathStrokeType(1.0));

    if (_analysis.rmsBins.empty()) { return; }

    auto max = *std::max_element(_analysis.rmsBins.begin(), _analysis.rmsBins.end());

    for (auto i = 0U; i < _analysis.rmsBins.size(); ++i)
    {
        auto bin = _analysis.rmsBins[i];
        if (bin == 0) { continue; }
        auto const fsize  = static_cast<float>(_analysis.rmsBins.size());
        auto const height = _rmsBinsArea.getHeight() * ((float)bin / (float)max);
        auto const x      = (fsize - static_cast<float>(i)) / fsize * _rmsBinsArea.getWidth();
        g.fillRect(juce::Rectangle<float>(x, 0, 8.0f, height).withBottomY(_rmsBinsArea.getBottom()));

        auto textArea = juce::Rectangle<float>(x, 0, 40, 40).withBottomY(_rmsBinsArea.getBottom());
        g.drawText(juce::String(i), textArea, juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area                = getLocalBounds();
    auto const controlHeight = area.proportionOfHeight(0.06f);
    _loadFile.setBounds(area.removeFromTop(controlHeight));
    _analyze.setBounds(area.removeFromTop(controlHeight));
    _rmsWindowLength.setBounds(area.removeFromBottom(controlHeight));

    auto const windowHeight = area.proportionOfHeight(0.5f);
    _rmsWindowsArea         = area.removeFromBottom(windowHeight).toFloat();
    _rmsBinsArea            = area.toFloat();
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
        auto options = ta::LoudnessAnalysis::Options{{}, windowSize};
        {
            juce::ScopedLock lock{_mutex};
            options.buffer = _audioBuffer;
        }

        auto analyzer = ta::LoudnessAnalysis{options};
        auto result   = analyzer();

        {
            juce::ScopedLock lock{_mutex};
            _analysis = std::move(result);
        }

        juce::MessageManager::callAsync([this] { repaint(); });
    };

    _threadPool.addJob(task);
}