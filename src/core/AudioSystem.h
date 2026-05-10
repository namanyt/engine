#pragma once

#include "core/AudioCategory.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace engine
{
class AudioAsset;

class AudioHandle final
{
  public:
    AudioHandle() = default;
    explicit AudioHandle(std::uint64_t value) : m_value(value) {}

    bool valid() const noexcept
    {
        return m_value != 0;
    }

    explicit operator bool() const noexcept
    {
        return valid();
    }

    std::uint64_t value() const noexcept
    {
        return m_value;
    }

    friend bool operator==(AudioHandle left, AudioHandle right) noexcept
    {
        return left.m_value == right.m_value;
    }

    friend bool operator!=(AudioHandle left, AudioHandle right) noexcept
    {
        return !(left == right);
    }

  private:
    std::uint64_t m_value = 0;
};

class AudioSystem final
{
  public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    void initialize();
    void shutdown() noexcept;
    void update(float deltaSeconds) noexcept;

    AudioHandle play(const std::shared_ptr<AudioAsset>& audioAsset, bool looping = false,
                     float volume = 1.0f, AudioCategory category = AudioCategory::SFX);
    AudioHandle playPersistent(std::string persistentId,
                               const std::shared_ptr<AudioAsset>& audioAsset, bool looping = false,
                               float volume = 1.0f, AudioCategory category = AudioCategory::Music);
    bool stop(AudioHandle handle) noexcept;
    bool stopPersistent(std::string_view persistentId) noexcept;
    bool setVolume(AudioHandle handle, float volume) noexcept;
    void setMasterVolume(float volume) noexcept;
    void setCategoryVolume(AudioCategory category, float volume) noexcept;
    void setMusicVolume(float volume) noexcept;
    float masterVolume() const noexcept;
    float categoryVolume(AudioCategory category) const noexcept;
    float musicVolume() const noexcept;
    AudioHandle persistentHandle(std::string_view persistentId) const noexcept;
    bool fadeTo(AudioHandle handle, float targetVolume, float durationSeconds,
                bool stopWhenDone = false) noexcept;
    bool fadeOut(AudioHandle handle, float durationSeconds) noexcept;
    bool fadeOutPersistent(std::string_view persistentId, float durationSeconds) noexcept;
    bool isPlaying(AudioHandle handle) const noexcept;

  private:
    struct PlaybackSlot;

    PlaybackSlot* findPlayback(AudioHandle handle) noexcept;
    const PlaybackSlot* findPlayback(AudioHandle handle) const noexcept;
    void forgetPersistentHandle(AudioHandle handle) noexcept;
    void destroyPlayback(PlaybackSlot& playback) noexcept;
    void applyPlaybackVolume(PlaybackSlot& playback) noexcept;
    void applyGlobalVolumes() noexcept;

    bool m_initialized = false;
    float m_masterVolume = 1.0f;
    std::array<float, kAudioCategoryCount> m_categoryVolumes{1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    std::uint64_t m_nextHandleValue = 1;
    std::vector<std::unique_ptr<PlaybackSlot>> m_playbacks;
    std::unordered_map<std::string, AudioHandle> m_persistentHandles;
    void* m_engineStorage = nullptr;
};
} // namespace engine
