#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

namespace ta
{

struct BufferWithSampleRate
{
    BufferWithSampleRate() = default;
    BufferWithSampleRate(juce::AudioBuffer<float>&& bufferIn, double sampleRateIn);

    juce::AudioBuffer<float> buffer;
    double sampleRate = 0.0;
};

auto loadAudioFileToBuffer(juce::File const& file, size_t maxLength) -> BufferWithSampleRate;
auto resampleAudioBuffer(BufferWithSampleRate const& buffer, double targetSampleRate) -> BufferWithSampleRate;
}  // namespace ta
