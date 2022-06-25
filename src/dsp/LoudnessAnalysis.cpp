#include "LoudnessAnalysis.hpp"

namespace ta
{
namespace
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
        auto const peak        = buffer.buffer.getMagnitude(0, (int)windowStart, (int)windowSampleCount);
        auto const rms         = buffer.buffer.getRMSLevel(0, (int)windowStart, (int)windowSampleCount);
        rmsWindows.push_back(LevelWindow{peak, rms});
    }

    return rmsWindows;
}
}  // namespace

LoudnessAnalysis::LoudnessAnalysis(Options options) : _options{std::move(options)} {}

auto LoudnessAnalysis::operator()() -> Result
{
    auto rmsWindows = analyseFile(_options.buffer, _options.windowLength);

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

    return {std::move(rmsWindows), std::move(rmsBins)};
}

}  // namespace ta