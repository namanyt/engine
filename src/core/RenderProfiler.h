#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace engine
{
struct PassPerformanceStats final
{
    std::string name;
    float cpuMilliseconds = 0.0f;
    float gpuMilliseconds = 0.0f;
};

struct FramePerformanceStats final
{
    float cpuFrameMilliseconds = 0.0f;
    float gpuFrameMilliseconds = 0.0f;
    int drawCallCount = 0;
    std::uint64_t volumetricSampleCount = 0;
    int raymarchStepCount = 0;
    int volumetricBufferWidth = 0;
    int volumetricBufferHeight = 0;
    std::vector<PassPerformanceStats> passes{};
};

class RenderProfiler final
{
  public:
    class ScopedCpuSample;
    class ScopedGpuSample;

    RenderProfiler();
    ~RenderProfiler();

    RenderProfiler(const RenderProfiler&) = delete;
    RenderProfiler& operator=(const RenderProfiler&) = delete;
    RenderProfiler(RenderProfiler&&) = delete;
    RenderProfiler& operator=(RenderProfiler&&) = delete;

    void beginFrame();
    void endFrame();

    ScopedCpuSample makeCpuScope(std::string_view name);
    ScopedGpuSample makeGpuScope(std::string_view name);

    void addDrawCall(int amount = 1) noexcept;
    void setVolumetricStats(int bufferWidth, int bufferHeight, int raymarchStepCount,
                            std::uint64_t sampleCount) noexcept;

    const FramePerformanceStats& stats() const noexcept;

  private:
    friend class ScopedCpuSample;
    friend class ScopedGpuSample;

    using Clock = std::chrono::high_resolution_clock;

    void recordCpuSample(std::string_view name, float milliseconds);
    void beginGpuSample(std::string_view name);
    void endGpuSample(std::string_view name);

    struct CpuPassState final
    {
        std::string name;
        float milliseconds = 0.0f;
    };

    struct GpuPassState final
    {
        std::string name;
        unsigned int startQueries[3] = {0, 0, 0};
        unsigned int endQueries[3] = {0, 0, 0};
        bool pending[3] = {false, false, false};
        float lastMilliseconds = 0.0f;
        bool wasUsed = false;
    };

    GpuPassState& assureGpuPass(std::string_view name);
    void resolveGpuSlot(unsigned int slot);
    void rebuildSnapshot();

    Clock::time_point m_frameStartTime{};
    std::vector<CpuPassState> m_cpuPasses;
    std::vector<GpuPassState> m_gpuPasses;
    FramePerformanceStats m_stats{};
    int m_drawCallCount = 0;
    unsigned int m_frameIndex = 0;
    unsigned int m_currentGpuSlot = 0;
};

class RenderProfiler::ScopedCpuSample final
{
  public:
    ScopedCpuSample(RenderProfiler& profiler, std::string_view name);
    ~ScopedCpuSample();

    ScopedCpuSample(const ScopedCpuSample&) = delete;
    ScopedCpuSample& operator=(const ScopedCpuSample&) = delete;
    ScopedCpuSample(ScopedCpuSample&&) = delete;
    ScopedCpuSample& operator=(ScopedCpuSample&&) = delete;

  private:
    RenderProfiler* m_profiler = nullptr;
    std::string m_name;
    Clock::time_point m_startTime{};
};

class RenderProfiler::ScopedGpuSample final
{
  public:
    ScopedGpuSample(RenderProfiler& profiler, std::string_view name);
    ~ScopedGpuSample();

    ScopedGpuSample(const ScopedGpuSample&) = delete;
    ScopedGpuSample& operator=(const ScopedGpuSample&) = delete;
    ScopedGpuSample(ScopedGpuSample&&) = delete;
    ScopedGpuSample& operator=(ScopedGpuSample&&) = delete;

  private:
    RenderProfiler* m_profiler = nullptr;
    std::string m_name;
};
} // namespace engine
