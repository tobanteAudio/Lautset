#include "BufferWithSampleRate.hpp"

namespace ta
{

BufferWithSampleRate::BufferWithSampleRate(juce::AudioBuffer<float>&& bufferIn, double sampleRateIn)
    : buffer(std::move(bufferIn)), sampleRate(sampleRateIn)
{
}

auto loadAudioFileToBuffer(juce::File const& file, size_t maxLength) -> BufferWithSampleRate
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();

    juce::MemoryMappedFile mapped{file, juce::MemoryMappedFile::readOnly};
    auto imap = std::make_unique<juce::MemoryInputStream>(mapped.getData(), mapped.getSize(), false);
    std::unique_ptr<juce::AudioFormatReader> formatReader(manager.createReaderFor(std::move(imap)));

    if (formatReader == nullptr) return {};

    auto const fileLength   = static_cast<size_t>(formatReader->lengthInSamples);
    auto const lengthToLoad = maxLength == 0 ? fileLength : juce::jmin(maxLength, fileLength);

    BufferWithSampleRate result{
        juce::AudioBuffer<float>{
            juce::jlimit(1, 2, static_cast<int>(formatReader->numChannels)),
            static_cast<int>(lengthToLoad),
        },
        formatReader->sampleRate,
    };

    formatReader->read(result.buffer.getArrayOfWritePointers(), result.buffer.getNumChannels(), 0,
                       result.buffer.getNumSamples());

    return result;
}

auto resampleAudioBuffer(BufferWithSampleRate const& in, double targetSampleRate) -> BufferWithSampleRate
{
    if (in.sampleRate == targetSampleRate) { return in; }

    auto const factorReading = in.sampleRate / targetSampleRate;

    auto original = juce::AudioBuffer<float>{in.buffer};
    juce::MemoryAudioSource memorySource(original, false);
    juce::ResamplingAudioSource resamplingSource(&memorySource, false, in.buffer.getNumChannels());

    auto const finalSize = juce::roundToInt(juce::jmax(1.0, in.buffer.getNumSamples() / factorReading));
    resamplingSource.setResamplingRatio(factorReading);
    resamplingSource.prepareToPlay(finalSize, in.sampleRate);

    auto out = BufferWithSampleRate{juce::AudioBuffer<float>(in.buffer.getNumChannels(), finalSize), targetSampleRate};
    resamplingSource.getNextAudioBlock({&out.buffer, 0, out.buffer.getNumSamples()});

    return out;
}

}  // namespace ta
