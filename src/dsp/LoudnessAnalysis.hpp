#pragma once

#include "dsp/BufferWithSampleRate.hpp"
#include "dsp/Duration.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace ta
{
struct LevelWindow
{
    float peak{};
    float rms{};
};

struct LoudnessAnalysisOptions
{
    std::shared_ptr<BufferWithSampleRate const> buffer;
    Milliseconds<float> windowLength{};
};

struct LoudnessAnalysisResult
{
    std::vector<LevelWindow> rmsWindows;
    std::vector<int> rmsBins;
};

struct LoudnessAnalysis
{
    using Options = LoudnessAnalysisOptions;
    using Result  = LoudnessAnalysisResult;

    explicit LoudnessAnalysis(Options options);
    auto operator()() -> Result;

private:
    Options _options;
};

}  // namespace ta