#include "MainComponent.hpp"

#include "dsp/BufferWithSampleRate.hpp"
#include "dsp/Duration.hpp"

namespace ta
{

auto analyseFile(juce::File const& path) -> std::vector<LevelWindow>
{
    DBG("Load");
    if (!path.existsAsFile()) return {};
    auto const buffer = loadAudioFileToBuffer(path, 0);
    if (buffer.buffer.getNumSamples() == 0) return {};
    DBG("Load Finished");

    auto const windowLength      = Milliseconds<float>{10000.0f};
    auto const windowSampleCount = static_cast<std::size_t>(toSampleCount(windowLength, buffer.sampleRate));
    auto const numWindows        = static_cast<std::size_t>(buffer.buffer.getNumSamples()) / windowSampleCount;

    DBG("Window length: " + juce::String{toMilliseconds(windowLength).count()} + " ms");
    DBG("Num Window:: " + juce::String{numWindows});

    DBG("RMS Calc Start");
    auto rmsWindows = std::vector<LevelWindow>{};
    rmsWindows.reserve(numWindows);

    for (auto i{0U}; i < numWindows; ++i)
    {
        auto const windowStart = i * windowSampleCount;
        auto const peak        = buffer.buffer.getMagnitude(0, windowStart, windowSampleCount);
        auto const rms         = buffer.buffer.getRMSLevel(0, windowStart, windowSampleCount);
        rmsWindows.push_back(LevelWindow{peak, rms});
    }
    DBG("RMS Calc Finished");

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

auto peakWindowsToPath(std::vector<ta::LevelWindow> const& windows, juce::Rectangle<int> area) -> juce::Path
{
    return containerToPath(windows.size(), area, [&windows](std::size_t i) { return windows[i].peak; });
}

auto rmsWindowsToPath(std::vector<ta::LevelWindow> const& windows, juce::Rectangle<int> area) -> juce::Path
{
    return containerToPath(windows.size(), area, [&windows](std::size_t i) { return windows[i].rms; });
}

auto rmsAverageToPath(std::vector<float> const& averages, juce::Rectangle<int> area) -> juce::Path
{
    return containerToPath(averages.size(), area, [&averages](std::size_t i) { return averages[i]; });
}

template<typename ContainerAccess>
auto average(std::size_t count, ContainerAccess access) -> float
{
    auto sum = 0.0f;
    for (auto i{0U}; i < count; ++i) { sum += access(i); }
    return sum / static_cast<float>(count);
}

template<typename ContainerAccess>
auto rollingAverage(std::size_t count, float* out, ContainerAccess access) -> void
{
    for (auto i{0U}; i < count; ++i, ++out) { *out = average(i + 1, access); }
}

auto rollingAverage(std::vector<ta::LevelWindow> const& windows) -> std::vector<float>
{
    auto averages = std::vector<float>{};
    averages.resize(windows.size());
    rollingAverage(windows.size(), averages.data(), [&windows](std::size_t i) { return windows[i].rms; });
    return averages;
}

MainComponent::MainComponent()
{
    addAndMakeVisible(_loadFile);
    _loadFile.onClick = [this]
    {
        auto path   = juce::File{"/home/tobante/Downloads/3 deck action volume adjust mp3.mp3"};
        _rmsWindows = ta::analyseFile(path);
        repaint();
    };

    _formatManager.registerBasicFormats();
    setSize(600, 400);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.fillRect(_drawArea);

    auto const peakPath = peakWindowsToPath(_rmsWindows, _drawArea);
    g.setColour(juce::Colours::red);
    g.strokePath(peakPath, juce::PathStrokeType(1.0));

    auto const rmsPath = rmsWindowsToPath(_rmsWindows, _drawArea);
    g.setColour(juce::Colours::black);
    g.strokePath(rmsPath, juce::PathStrokeType(1.0));

    auto const rmsAveragePath = rmsAverageToPath(rollingAverage(_rmsWindows), _drawArea);
    g.setColour(juce::Colours::blue);
    g.strokePath(rmsAveragePath, juce::PathStrokeType(1.0));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    _loadFile.setBounds(area.removeFromTop(area.proportionOfHeight(0.1f)));
    _drawArea = area;
}
