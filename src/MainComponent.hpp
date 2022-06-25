#pragma once

#include "dsp/BufferWithSampleRate.hpp"
#include "dsp/Duration.hpp"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace ta
{
struct LevelWindow
{
    float peak{};
    float rms{};
};
}  // namespace ta

class MainComponent : public juce::Component
{
public:
    MainComponent();

    auto paint(juce::Graphics&) -> void override;
    auto resized() -> void override;

private:
    auto loadFile(juce::File const& file) -> void;
    auto analyseAudio() -> void;

    juce::ThreadPool _threadPool{juce::SystemStats::getNumCpus()};

    juce::CriticalSection _mutex;
    juce::AudioFormatManager _formatManager;
    juce::TextButton _loadFile{"Load File"};
    juce::TextButton _analyze{"Analyze"};
    juce::Slider _rmsWindowLength{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Rectangle<int> _rmsWindowsArea{};
    juce::Rectangle<int> _rmsBinsArea{};

    ta::BufferWithSampleRate _audioBuffer{};
    std::vector<ta::LevelWindow> _rmsWindows;

    std::vector<int> _rmsBins;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
