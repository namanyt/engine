#include "core/RenderProfiler.h"

#include <glad/glad.h>

#include <algorithm>

namespace
{
constexpr unsigned int kGpuQueryLatency = 3;

bool supportsGpuTimestamps() noexcept
{
    return glQueryCounter != nullptr && glGetQueryObjectui64v != nullptr;
}

float nanosecondsToMilliseconds(unsigned long long nanoseconds) noexcept
{
    return static_cast<float>(static_cast<double>(nanoseconds) / 1000000.0);
}
} // namespace

namespace engine
{
RenderProfiler::RenderProfiler() = default;

RenderProfiler::~RenderProfiler()
{
    for (GpuPassState& pass : m_gpuPasses)
    {
        if (pass.startQueries[0] != 0)
        {
            glDeleteQueries(static_cast<GLsizei>(kGpuQueryLatency), pass.startQueries);
        }

        if (pass.endQueries[0] != 0)
        {
            glDeleteQueries(static_cast<GLsizei>(kGpuQueryLatency), pass.endQueries);
        }
    }
}

void RenderProfiler::beginFrame()
{
    m_currentGpuSlot = m_frameIndex % kGpuQueryLatency;
    if (supportsGpuTimestamps() && m_frameIndex >= kGpuQueryLatency)
    {
        resolveGpuSlot(m_currentGpuSlot);
    }

    m_cpuPasses.clear();
    m_drawCallCount = 0;
    m_stats.volumetricBufferWidth = 0;
    m_stats.volumetricBufferHeight = 0;
    m_stats.raymarchStepCount = 0;
    m_stats.volumetricSampleCount = 0;
    m_frameStartTime = Clock::now();
}

void RenderProfiler::endFrame()
{
    m_stats.cpuFrameMilliseconds =
        std::chrono::duration<float, std::milli>(Clock::now() - m_frameStartTime).count();
    m_stats.drawCallCount = m_drawCallCount;
    rebuildSnapshot();
    ++m_frameIndex;
}

RenderProfiler::ScopedCpuSample RenderProfiler::makeCpuScope(std::string_view name)
{
    return ScopedCpuSample(*this, name);
}

RenderProfiler::ScopedGpuSample RenderProfiler::makeGpuScope(std::string_view name)
{
    return ScopedGpuSample(*this, name);
}

void RenderProfiler::addDrawCall(int amount) noexcept
{
    m_drawCallCount += amount;
}

void RenderProfiler::setVolumetricStats(int bufferWidth, int bufferHeight, int raymarchStepCount,
                                        std::uint64_t sampleCount) noexcept
{
    m_stats.volumetricBufferWidth = bufferWidth;
    m_stats.volumetricBufferHeight = bufferHeight;
    m_stats.raymarchStepCount = raymarchStepCount;
    m_stats.volumetricSampleCount = sampleCount;
}

const FramePerformanceStats& RenderProfiler::stats() const noexcept
{
    return m_stats;
}

void RenderProfiler::recordCpuSample(std::string_view name, float milliseconds)
{
    for (CpuPassState& pass : m_cpuPasses)
    {
        if (pass.name == name)
        {
            pass.milliseconds += milliseconds;
            return;
        }
    }

    m_cpuPasses.push_back(CpuPassState{std::string{name}, milliseconds});
}

void RenderProfiler::beginGpuSample(std::string_view name)
{
    if (!supportsGpuTimestamps())
    {
        return;
    }

    GpuPassState& pass = assureGpuPass(name);
    glQueryCounter(pass.startQueries[m_currentGpuSlot], GL_TIMESTAMP);
    pass.pending[m_currentGpuSlot] = true;
    pass.wasUsed = true;
}

void RenderProfiler::endGpuSample(std::string_view name)
{
    if (!supportsGpuTimestamps())
    {
        return;
    }

    GpuPassState& pass = assureGpuPass(name);
    glQueryCounter(pass.endQueries[m_currentGpuSlot], GL_TIMESTAMP);
}

RenderProfiler::GpuPassState& RenderProfiler::assureGpuPass(std::string_view name)
{
    for (GpuPassState& pass : m_gpuPasses)
    {
        if (pass.name == name)
        {
            return pass;
        }
    }

    GpuPassState& pass = m_gpuPasses.emplace_back();
    pass.name = std::string{name};
    if (supportsGpuTimestamps())
    {
        glGenQueries(static_cast<GLsizei>(kGpuQueryLatency), pass.startQueries);
        glGenQueries(static_cast<GLsizei>(kGpuQueryLatency), pass.endQueries);
    }

    return pass;
}

void RenderProfiler::resolveGpuSlot(unsigned int slot)
{
    for (GpuPassState& pass : m_gpuPasses)
    {
        if (!pass.pending[slot])
        {
            continue;
        }

        unsigned long long startTime = 0;
        unsigned long long endTime = 0;
        glGetQueryObjectui64v(pass.startQueries[slot], GL_QUERY_RESULT, &startTime);
        glGetQueryObjectui64v(pass.endQueries[slot], GL_QUERY_RESULT, &endTime);
        pass.lastMilliseconds =
            endTime >= startTime ? nanosecondsToMilliseconds(endTime - startTime) : 0.0f;
        pass.pending[slot] = false;
    }
}

void RenderProfiler::rebuildSnapshot()
{
    m_stats.passes.clear();

    for (const CpuPassState& cpuPass : m_cpuPasses)
    {
        m_stats.passes.push_back(PassPerformanceStats{cpuPass.name, cpuPass.milliseconds, 0.0f});
    }

    float accumulatedGpuMilliseconds = 0.0f;
    for (const GpuPassState& gpuPass : m_gpuPasses)
    {
        if (!gpuPass.wasUsed)
        {
            continue;
        }

        accumulatedGpuMilliseconds += gpuPass.lastMilliseconds;
        auto existing = std::find_if(m_stats.passes.begin(), m_stats.passes.end(),
                                     [&](const PassPerformanceStats& pass)
                                     { return pass.name == gpuPass.name; });
        if (existing != m_stats.passes.end())
        {
            existing->gpuMilliseconds = gpuPass.lastMilliseconds;
        }
        else
        {
            m_stats.passes.push_back(
                PassPerformanceStats{gpuPass.name, 0.0f, gpuPass.lastMilliseconds});
        }

        if (gpuPass.name == "Frame")
        {
            m_stats.gpuFrameMilliseconds = gpuPass.lastMilliseconds;
        }
    }

    if (m_stats.gpuFrameMilliseconds <= 0.0f)
    {
        m_stats.gpuFrameMilliseconds = accumulatedGpuMilliseconds;
    }
}

RenderProfiler::ScopedCpuSample::ScopedCpuSample(RenderProfiler& profiler, std::string_view name)
    : m_profiler(&profiler), m_name(name), m_startTime(Clock::now())
{
}

RenderProfiler::ScopedCpuSample::~ScopedCpuSample()
{
    if (m_profiler == nullptr)
    {
        return;
    }

    const float milliseconds =
        std::chrono::duration<float, std::milli>(Clock::now() - m_startTime).count();
    m_profiler->recordCpuSample(m_name, milliseconds);
}

RenderProfiler::ScopedGpuSample::ScopedGpuSample(RenderProfiler& profiler, std::string_view name)
    : m_profiler(&profiler), m_name(name)
{
    m_profiler->beginGpuSample(m_name);
}

RenderProfiler::ScopedGpuSample::~ScopedGpuSample()
{
    if (m_profiler != nullptr)
    {
        m_profiler->endGpuSample(m_name);
    }
}
} // namespace engine
