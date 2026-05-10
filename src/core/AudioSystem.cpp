#include "core/AudioSystem.h"

#include "assets/AudioAsset.h"
#include "core/Log.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace engine
{
namespace
{
float clampVolume(const float volume) noexcept
{
    return std::clamp(volume, 0.0f, 1.0f);
}

std::size_t categoryIndex(const AudioCategory category) noexcept
{
    switch (category)
    {
    case AudioCategory::Music:
        return 0;
    case AudioCategory::UI:
        return 1;
    case AudioCategory::VN:
        return 2;
    case AudioCategory::Ambient:
        return 3;
    case AudioCategory::SFX:
    default:
        return 4;
    }
}
} // namespace

struct AudioSystem::PlaybackSlot final
{
    struct FadeState final
    {
        float startVolume = 1.0f;
        float targetVolume = 1.0f;
        float durationSeconds = 0.0f;
        float elapsedSeconds = 0.0f;
        bool stopWhenDone = false;
        bool active = false;
    };

    AudioHandle handle{};
    std::shared_ptr<AudioAsset> asset;
    ma_decoder decoder{};
    ma_sound sound{};
    float baseVolume = 1.0f;
    float currentVolume = 1.0f;
    AudioCategory category = AudioCategory::SFX;
    bool looping = false;
    bool decoderInitialized = false;
    bool soundInitialized = false;
    FadeState fade{};
};

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem()
{
    shutdown();
}

void AudioSystem::initialize()
{
    if (m_initialized)
    {
        return;
    }

    auto engine = std::make_unique<ma_engine>();
    const ma_result result = ma_engine_init(nullptr, engine.get());
    if (result != MA_SUCCESS)
    {
        throw std::runtime_error("Failed to initialize miniaudio engine.");
    }

    m_engineStorage = engine.release();
    m_initialized = true;
    Log::info("AudioSystem", "Initialized miniaudio engine.");
}

void AudioSystem::shutdown() noexcept
{
    if (!m_initialized)
    {
        return;
    }

    for (const std::unique_ptr<PlaybackSlot>& playback : m_playbacks)
    {
        destroyPlayback(*playback);
    }

    m_playbacks.clear();

    if (m_engineStorage != nullptr)
    {
        ma_engine_uninit(reinterpret_cast<ma_engine*>(m_engineStorage));
        delete reinterpret_cast<ma_engine*>(m_engineStorage);
        m_engineStorage = nullptr;
    }

    m_initialized = false;
    Log::info("AudioSystem", "Audio engine shutdown complete.");
}

void AudioSystem::update(const float deltaSeconds) noexcept
{
    if (!m_initialized || m_playbacks.empty())
    {
        return;
    }

    for (auto iterator = m_playbacks.begin(); iterator != m_playbacks.end();)
    {
        PlaybackSlot& playback = *(*iterator);
        bool removePlayback = false;

        if (playback.fade.active && playback.soundInitialized)
        {
            playback.fade.elapsedSeconds += std::max(deltaSeconds, 0.0f);

            const float fadeProgress =
                playback.fade.durationSeconds <= 0.0f
                    ? 1.0f
                    : std::clamp(playback.fade.elapsedSeconds / playback.fade.durationSeconds, 0.0f,
                                 1.0f);
            playback.baseVolume =
                playback.fade.startVolume +
                (playback.fade.targetVolume - playback.fade.startVolume) * fadeProgress;
            applyPlaybackVolume(playback);

            if (fadeProgress >= 1.0f)
            {
                playback.fade.active = false;
                if (playback.fade.stopWhenDone)
                {
                    removePlayback = true;
                }
            }
        }

        if (!removePlayback && playback.soundInitialized && !playback.looping &&
            ma_sound_is_playing(&playback.sound) != MA_TRUE)
        {
            removePlayback = true;
        }

        if (removePlayback)
        {
            destroyPlayback(playback);
            iterator = m_playbacks.erase(iterator);
            continue;
        }

        ++iterator;
    }
}

AudioHandle AudioSystem::play(const std::shared_ptr<AudioAsset>& audioAsset, const bool looping,
                              const float volume, const AudioCategory category)
{
    if (!m_initialized)
    {
        throw std::runtime_error("AudioSystem::play() called before initialize().");
    }

    if (audioAsset == nullptr)
    {
        throw std::runtime_error("AudioSystem::play() requires a valid AudioAsset.");
    }

    auto playback = std::make_unique<PlaybackSlot>();
    playback->handle = AudioHandle(m_nextHandleValue++);
    playback->asset = audioAsset;
    playback->looping = looping;
    playback->category = category;

    const std::vector<std::uint8_t>& encodedBytes = audioAsset->encodedBytes();
    const ma_result decoderResult = ma_decoder_init_memory(encodedBytes.data(), encodedBytes.size(),
                                                           nullptr, &playback->decoder);
    if (decoderResult != MA_SUCCESS)
    {
        throw std::runtime_error("Failed to initialize audio decoder for asset: " +
                                 audioAsset->sourcePath().string());
    }

    playback->decoderInitialized = true;

    ma_uint32 flags = MA_SOUND_FLAG_STREAM;
    if (looping)
    {
        flags |= MA_SOUND_FLAG_LOOPING;
    }

    const ma_result soundResult =
        ma_sound_init_from_data_source(reinterpret_cast<ma_engine*>(m_engineStorage),
                                       &playback->decoder, flags, nullptr, &playback->sound);
    if (soundResult != MA_SUCCESS)
    {
        destroyPlayback(*playback);
        throw std::runtime_error("Failed to initialize audio playback for asset: " +
                                 audioAsset->sourcePath().string());
    }

    playback->soundInitialized = true;
    playback->baseVolume = clampVolume(volume);
    applyPlaybackVolume(*playback);

    const ma_result startResult = ma_sound_start(&playback->sound);
    if (startResult != MA_SUCCESS)
    {
        destroyPlayback(*playback);
        throw std::runtime_error("Failed to start audio playback for asset: " +
                                 audioAsset->sourcePath().string());
    }

    std::ostringstream stream;
    stream << "Started " << (looping ? "looping" : "one-shot")
           << " audio playback: " << audioAsset->sourcePath().filename().string() << " (handle "
           << playback->handle.value() << ").";
    Log::info("AudioSystem", stream.str());

    const AudioHandle handle = playback->handle;
    m_playbacks.push_back(std::move(playback));
    return handle;
}

AudioHandle AudioSystem::playPersistent(std::string persistentId,
                                        const std::shared_ptr<AudioAsset>& audioAsset,
                                        const bool looping, const float volume,
                                        const AudioCategory category)
{
    if (persistentId.empty())
    {
        throw std::runtime_error("AudioSystem::playPersistent() requires a persistent ID.");
    }

    const auto existing = m_persistentHandles.find(persistentId);
    if (existing != m_persistentHandles.end())
    {
        if (PlaybackSlot* playback = findPlayback(existing->second); playback != nullptr)
        {
            playback->category = category;
            playback->baseVolume = clampVolume(volume);
            playback->fade.active = false;
            applyPlaybackVolume(*playback);
            return playback->handle;
        }

        m_persistentHandles.erase(existing);
    }

    const AudioHandle handle = play(audioAsset, looping, volume, category);
    m_persistentHandles.emplace(std::move(persistentId), handle);
    return handle;
}

bool AudioSystem::stop(const AudioHandle handle) noexcept
{
    const auto iterator = std::find_if(m_playbacks.begin(), m_playbacks.end(),
                                       [handle](const std::unique_ptr<PlaybackSlot>& playback)
                                       { return playback->handle == handle; });
    if (iterator == m_playbacks.end())
    {
        return false;
    }

    forgetPersistentHandle(handle);
    destroyPlayback(*(*iterator));
    m_playbacks.erase(iterator);
    return true;
}

bool AudioSystem::stopPersistent(const std::string_view persistentId) noexcept
{
    const AudioHandle handle = persistentHandle(persistentId);
    return handle ? stop(handle) : false;
}

bool AudioSystem::setVolume(const AudioHandle handle, const float volume) noexcept
{
    PlaybackSlot* playback = findPlayback(handle);
    if (playback == nullptr || !playback->soundInitialized)
    {
        return false;
    }

    playback->baseVolume = clampVolume(volume);
    playback->fade.active = false;
    applyPlaybackVolume(*playback);
    return true;
}

void AudioSystem::setMasterVolume(const float volume) noexcept
{
    m_masterVolume = clampVolume(volume);
    applyGlobalVolumes();
}

void AudioSystem::setCategoryVolume(const AudioCategory category, const float volume) noexcept
{
    m_categoryVolumes[categoryIndex(category)] = clampVolume(volume);
    applyGlobalVolumes();
}

void AudioSystem::setMusicVolume(const float volume) noexcept
{
    setCategoryVolume(AudioCategory::Music, volume);
}

float AudioSystem::masterVolume() const noexcept
{
    return m_masterVolume;
}

float AudioSystem::categoryVolume(const AudioCategory category) const noexcept
{
    return m_categoryVolumes[categoryIndex(category)];
}

float AudioSystem::musicVolume() const noexcept
{
    return categoryVolume(AudioCategory::Music);
}

AudioHandle AudioSystem::persistentHandle(const std::string_view persistentId) const noexcept
{
    const auto iterator = m_persistentHandles.find(std::string(persistentId));
    if (iterator == m_persistentHandles.end())
    {
        return {};
    }

    return findPlayback(iterator->second) != nullptr ? iterator->second : AudioHandle{};
}

bool AudioSystem::fadeTo(const AudioHandle handle, const float targetVolume,
                         const float durationSeconds, const bool stopWhenDone) noexcept
{
    PlaybackSlot* playback = findPlayback(handle);
    if (playback == nullptr || !playback->soundInitialized)
    {
        return false;
    }

    playback->fade.startVolume = playback->baseVolume;
    playback->fade.targetVolume = clampVolume(targetVolume);
    playback->fade.durationSeconds = std::max(durationSeconds, 0.0f);
    playback->fade.elapsedSeconds = 0.0f;
    playback->fade.stopWhenDone = stopWhenDone;
    playback->fade.active = true;

    if (playback->fade.durationSeconds <= 0.0f)
    {
        playback->baseVolume = playback->fade.targetVolume;
        applyPlaybackVolume(*playback);
        playback->fade.active = false;
        if (playback->fade.stopWhenDone)
        {
            return stop(handle);
        }
    }

    return true;
}

bool AudioSystem::fadeOut(const AudioHandle handle, const float durationSeconds) noexcept
{
    return fadeTo(handle, 0.0f, durationSeconds, true);
}

bool AudioSystem::fadeOutPersistent(const std::string_view persistentId,
                                    const float durationSeconds) noexcept
{
    const AudioHandle handle = persistentHandle(persistentId);
    return handle ? fadeOut(handle, durationSeconds) : false;
}

bool AudioSystem::isPlaying(const AudioHandle handle) const noexcept
{
    const PlaybackSlot* playback = findPlayback(handle);
    return playback != nullptr && playback->soundInitialized &&
           ma_sound_is_playing(&playback->sound) == MA_TRUE;
}

AudioSystem::PlaybackSlot* AudioSystem::findPlayback(const AudioHandle handle) noexcept
{
    const auto iterator = std::find_if(m_playbacks.begin(), m_playbacks.end(),
                                       [handle](const std::unique_ptr<PlaybackSlot>& playback)
                                       { return playback->handle == handle; });
    return iterator != m_playbacks.end() ? iterator->get() : nullptr;
}

const AudioSystem::PlaybackSlot* AudioSystem::findPlayback(const AudioHandle handle) const noexcept
{
    const auto iterator = std::find_if(m_playbacks.begin(), m_playbacks.end(),
                                       [handle](const std::unique_ptr<PlaybackSlot>& playback)
                                       { return playback->handle == handle; });
    return iterator != m_playbacks.end() ? iterator->get() : nullptr;
}

void AudioSystem::forgetPersistentHandle(const AudioHandle handle) noexcept
{
    for (auto iterator = m_persistentHandles.begin(); iterator != m_persistentHandles.end();)
    {
        if (iterator->second == handle)
        {
            iterator = m_persistentHandles.erase(iterator);
            continue;
        }

        ++iterator;
    }
}

void AudioSystem::destroyPlayback(PlaybackSlot& playback) noexcept
{
    if (playback.soundInitialized)
    {
        ma_sound_stop(&playback.sound);
        ma_sound_uninit(&playback.sound);
        playback.soundInitialized = false;
    }

    if (playback.decoderInitialized)
    {
        ma_decoder_uninit(&playback.decoder);
        playback.decoderInitialized = false;
    }

    playback.asset.reset();
}

void AudioSystem::applyPlaybackVolume(PlaybackSlot& playback) noexcept
{
    if (!playback.soundInitialized)
    {
        return;
    }

    playback.currentVolume =
        clampVolume(playback.baseVolume * categoryVolume(playback.category) * m_masterVolume);
    ma_sound_set_volume(&playback.sound, playback.currentVolume);
}

void AudioSystem::applyGlobalVolumes() noexcept
{
    for (const std::unique_ptr<PlaybackSlot>& playback : m_playbacks)
    {
        applyPlaybackVolume(*playback);
    }
}
} // namespace engine
