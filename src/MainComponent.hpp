#pragma once

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
    juce::AudioFormatManager _formatManager;
    juce::TextButton _loadFile{"Load File"};
    juce::Rectangle<int> _drawArea{};

    std::vector<ta::LevelWindow> _rmsWindows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
