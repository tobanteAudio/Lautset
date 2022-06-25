#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

namespace ta
{

struct BufferWithSampleRate
{
    juce::AudioBuffer<float> buffer;
    double sampleRate = 0.0;
};

auto loadAudioFileToBuffer(juce::File const& file, size_t maxLength) -> BufferWithSampleRate;

auto resampleAudioBuffer(BufferWithSampleRate const& in, double targetSampleRate) -> BufferWithSampleRate;

}  // namespace ta
