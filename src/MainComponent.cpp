#include "MainComponent.hpp"

#include <juce_dsp/juce_dsp.h>

namespace ta
{

namespace
{

auto positionForGain(float gain, float top, float bottom) -> float
{
    static constexpr auto minDB = -30.0F;
    static constexpr auto maxDB = 0.1F;
    return juce::jmap(juce::Decibels::gainToDecibels(gain, minDB), minDB, maxDB, bottom, top);
}

template<typename ContainerAccess>
auto containerToPath(std::size_t size, juce::Rectangle<float> area, ContainerAccess access) -> juce::Path
{
    if (size == 0U) { return {}; }

    juce::Path path{};
    path.preallocateSpace(static_cast<int>(size));

    auto const firstY = positionForGain(access(0U), area.getY(), area.getBottom());
    path.startNewSubPath(juce::Point<float>(0.0F, firstY));

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
MainComponent::MainComponent()
{
    _formatManager.registerBasicFormats();

    addAndMakeVisible(_loadFile);
    addAndMakeVisible(_rmsWindowLength);
    addAndMakeVisible(_rmsThreshold);

    _rmsWindowLength.setRange(juce::Range<double>{1'000.0, 45'000.0}, 100.0);
    _rmsWindowLength.setValue(5'000.0);
    _rmsWindowLength.onDragEnd = [this] { triggerAsyncUpdate(); };

    _rmsThreshold.setRange(juce::Range<double>{-30.0, 0.0}, 1.0);
    _rmsThreshold.setValue(-18.0);
    _rmsThreshold.onDragEnd = [this] { triggerAsyncUpdate(); };

    _loadFile.onClick = [this] { launchFileOpenDialog(); };
    _thumbnail.addChangeListener(this);

    setSize(1280, 720);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.fillRect(_rmsWindowsArea);
    g.fillRect(_rmsBinsArea);
    g.fillRect(_thumbnailArea);

    g.setColour(juce::Colours::black);
    _thumbnail.drawChannels(g, _thumbnailArea.toNearestInt(), 0.0, _thumbnail.getTotalLength(), 1.0F);

    if (_analysis.rmsWindows.empty()) { return; }

    auto const numWindows = _analysis.rmsWindows.size();
    auto const numBins    = _analysis.rmsBins.size();

    auto windowWidth = _thumbnailArea.getWidth() / (float)numWindows;
    auto threshold   = _rmsThreshold.getValue();
    for (auto i{0U}; i < numWindows; ++i)
    {
        auto dB   = juce::Decibels::gainToDecibels(_analysis.rmsWindows[i].rms);
        auto area = _thumbnailArea.withWidth(windowWidth).withX((float)i * windowWidth);
        g.setColour(dB <= threshold ? juce::Colours::green.withAlpha(0.3F) : juce::Colours::red.withAlpha(0.3F));
        g.fillRect(area);
    }

    auto rmsPath = rmsWindowsToPath(_analysis.rmsWindows, _rmsWindowsArea);
    g.setColour(juce::Colours::black);
    g.strokePath(rmsPath, juce::PathStrokeType(1.0));

    if (_analysis.rmsBins.empty()) { return; }

    auto max = *std::max_element(_analysis.rmsBins.begin(), _analysis.rmsBins.end());

    for (auto i{0U}; i < numBins; ++i)
    {
        auto bin = _analysis.rmsBins[i];
        if (bin == 0) { continue; }
        auto const fsize  = static_cast<float>(numBins);
        auto const height = _rmsBinsArea.getHeight() * ((float)bin / (float)max);
        auto const x      = (fsize - static_cast<float>(i)) / fsize * _rmsBinsArea.getWidth();
        g.fillRect(juce::Rectangle<float>(x, 0, 8.0F, height).withBottomY(_rmsBinsArea.getBottom()));

        auto textArea = juce::Rectangle<float>(x, 0, 40, 40).withBottomY(_rmsBinsArea.getBottom());
        g.drawText(juce::String(i), textArea, juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area                = getLocalBounds();
    auto const controlHeight = area.proportionOfHeight(0.06F);
    _loadFile.setBounds(area.removeFromTop(controlHeight));
    _rmsWindowLength.setBounds(area.removeFromBottom(controlHeight));
    _rmsThreshold.setBounds(area.removeFromBottom(controlHeight));

    auto const windowHeight = area.proportionOfHeight(0.3F);
    _thumbnailArea          = area.removeFromTop(windowHeight).toFloat();
    _rmsWindowsArea         = area.removeFromTop(windowHeight).toFloat();
    _rmsBinsArea            = area.toFloat();
}

auto MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source) -> void
{
    if (source == &_thumbnail) { repaint(); }
}
auto MainComponent::handleAsyncUpdate() -> void { analyseAudio(); }

auto MainComponent::loadFile(juce::File const& file) -> void
{
    _thumbnail.setSource(std::make_unique<juce::FileInputSource>(file).release());

    auto task = [this, path = file]
    {
        if (!path.existsAsFile()) { return; }

        auto audioBuffer = resampleAudioBuffer(loadAudioFileToBuffer(path, 0), 44'100.0 / 8.0);
        if (audioBuffer.buffer.getNumSamples() == 0) { return; }

        auto ptr = std::make_shared<BufferWithSampleRate const>(std::move(audioBuffer));

        juce::MessageManager::callAsync(
            [this, ptr]
            {
                _audioBuffer = ptr;
                triggerAsyncUpdate();
            });
    };

    _threadPool.addJob(task);
}

auto MainComponent::analyseAudio() -> void
{
    if (_audioBuffer == nullptr) { return; }

    auto windowSize = Milliseconds<float>{_rmsWindowLength.getValue()};
    auto task       = [this, windowSize, buffer = _audioBuffer]
    {
        auto analyzer = LoudnessAnalysis{{buffer, windowSize}};
        auto result   = analyzer();

        juce::MessageManager::callAsync(
            [this, result = std::move(result)]
            {
                _analysis = result;
                repaint();
            });
    };

    _threadPool.addJob(task);
}

auto MainComponent::launchFileOpenDialog() -> void
{
    auto const* msg      = "Please select the audio file you want to load...";
    auto const dir       = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
    auto const fileFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    auto load = [this](auto const& chooser)
    {
        auto file = chooser.getResult();
        if (!file.existsAsFile()) { return; }
        loadFile(file);
    };

    _fileChooser = std::make_unique<juce::FileChooser>(msg, dir, _formatManager.getWildcardForAllFormats());
    _fileChooser->launchAsync(fileFlags, load);
}

}  // namespace ta
