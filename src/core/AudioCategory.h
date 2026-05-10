#pragma once

#include <array>
#include <cstddef>

namespace engine
{
enum class AudioCategory
{
    Music,
    UI,
    VN,
    Ambient,
    SFX,
};

constexpr std::size_t kAudioCategoryCount = 5;

constexpr std::array<AudioCategory, kAudioCategoryCount> kAllAudioCategories{
    AudioCategory::Music,   AudioCategory::UI,  AudioCategory::VN,
    AudioCategory::Ambient, AudioCategory::SFX,
};
} // namespace engine
