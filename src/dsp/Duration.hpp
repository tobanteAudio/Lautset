#pragma once

#include <chrono>

namespace ta
{

/// \brief Duration.
template<typename T, typename R>
using Duration = std::chrono::duration<T, R>;

/// \brief Duration in microseconds.
template<typename T>
using Microseconds = Duration<T, std::micro>;

/// \brief Duration in milliseconds.
template<typename T>
using Milliseconds = Duration<T, std::milli>;

/// \brief Duration in seconds.
template<typename T>
using Seconds = Duration<T, std::ratio<1>>;

/// \brief Converts a duration to milliseconds.
template<typename T, typename R>
constexpr auto toMilliseconds(Duration<T, R> const& d) noexcept
{
    return std::chrono::duration_cast<Milliseconds<T>>(d);
}

/// \brief Converts a duration to seconds.
template<typename T, typename R>
constexpr auto toSeconds(Duration<T, R> const& d) noexcept
{
    return std::chrono::duration_cast<Seconds<T>>(d);
}

/// \brief Converts the duration to a sample count based on the sample-rate.
/// \see Duration
template<typename T, typename R>
auto toSampleCount(Duration<T, R> const& duration, double sampleRate) noexcept -> double
{
    return toSeconds(duration).count() * sampleRate;
}

}  // namespace ta
