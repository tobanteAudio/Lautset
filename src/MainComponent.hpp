#pragma once

#include "dsp/BufferWithSampleRate.hpp"
#include "dsp/Duration.hpp"
#include "dsp/LoudnessAnalysis.hpp"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace ta
{

struct MainComponent final
    : juce::Component
    , juce::ChangeListener
    , juce::AsyncUpdater
{
    MainComponent();

    auto paint(juce::Graphics& g) -> void override;
    auto resized() -> void override;

private:
    auto changeListenerCallback(juce::ChangeBroadcaster* source) -> void override;
    auto handleAsyncUpdate() -> void override;
    auto loadFile(juce::File const& file) -> void;
    auto analyseAudio() -> void;
    auto launchFileOpenDialog() -> void;

    juce::ThreadPool _threadPool{juce::SystemStats::getNumCpus()};
    juce::AudioFormatManager _formatManager;

    std::shared_ptr<BufferWithSampleRate const> _audioBuffer{};
    LoudnessAnalysis::Result _analysis{};

    juce::AudioThumbnailCache _thumbnailCache{1};
    juce::AudioThumbnail _thumbnail{1024, _formatManager, _thumbnailCache};

    juce::TextButton _loadFile{"Load File"};
    juce::Slider _rmsWindowLength{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Slider _rmsThreshold{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Rectangle<float> _rmsWindowsArea{};
    juce::Rectangle<float> _rmsBinsArea{};
    juce::Rectangle<float> _thumbnailArea{};

    std::unique_ptr<juce::FileChooser> _fileChooser{};
};

}  // namespace ta