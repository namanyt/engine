#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uHalfAtmosphereTexture;
uniform sampler2D uHalfMetricsTexture;
uniform sampler2D uHalfAuxTexture;
uniform sampler2D uHalfDepthTexture;
uniform sampler2D uHalfTemporalTexture;
uniform sampler2D uSceneDepthTexture;
uniform mat4 uInverseProjection;
uniform float uBilateralDepthFactor;
uniform vec2 uFullResolution;
uniform vec2 uHalfResolution;
uniform int uVolumetricDebugViewMode;

struct VolumetricSample
{
    vec4 atmosphere;
    vec4 metrics;
    vec4 aux;
    vec4 temporal;
};

struct ResolvedVolumetricSample
{
    VolumetricSample sample;
    float edgeMask;
};

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 heatmapColor(float value)
{
    float clamped = clamp(value, 0.0, 1.0);
    return clamp(vec3(1.5 * clamped,
                      1.5 * (1.0 - abs(clamped - 0.5) * 2.0),
                      1.5 * (1.0 - clamped)),
                 0.0, 1.0);
}

float linearDepth(vec2 uv, float deviceDepth)
{
    if (deviceDepth >= 0.999999)
    {
        return 1000000.0;
    }

    vec4 clip = vec4(uv * 2.0 - 1.0, deviceDepth * 2.0 - 1.0, 1.0);
    vec4 view = uInverseProjection * clip;
    return max(-view.z / max(view.w, 0.0001), 0.0);
}

VolumetricSample sampleNearest(ivec2 texel)
{
    VolumetricSample result;
    result.atmosphere = texelFetch(uHalfAtmosphereTexture, texel, 0);
    result.metrics = texelFetch(uHalfMetricsTexture, texel, 0);
    result.aux = texelFetch(uHalfAuxTexture, texel, 0);
    result.temporal = texelFetch(uHalfTemporalTexture, texel, 0);
    return result;
}

ResolvedVolumetricSample resolveBilateral(vec2 uv)
{
    vec2 halfPixel = uv * uHalfResolution - vec2(0.5);
    ivec2 center = ivec2(round(halfPixel));
    float currentDepth = linearDepth(uv, texture(uSceneDepthTexture, uv).r);
    bool hasGeometry = currentDepth < 999999.0;

    ResolvedVolumetricSample resolved;
    VolumetricSample accumulated;
    float totalWeight = 0.0;
    float totalSpatialWeight = 0.0;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            ivec2 texel = clamp(center + ivec2(x, y), ivec2(0), ivec2(uHalfResolution) - ivec2(1));
            VolumetricSample sampleValue = sampleNearest(texel);
            float sampleDepth = texelFetch(uHalfDepthTexture, texel, 0).r;
            vec2 texelOffset = vec2(texel) + vec2(0.5) - halfPixel;
            float spatialWeight = exp(-dot(texelOffset, texelOffset) * 0.8);
            totalSpatialWeight += spatialWeight;
            float depthThreshold = max(0.15, currentDepth * 0.01);
            bool depthAccepted = !hasGeometry || abs(sampleDepth - currentDepth) <= depthThreshold;
            float depthWeight = depthAccepted
                                    ? exp(-abs(sampleDepth - currentDepth) * uBilateralDepthFactor)
                                    : 0.0;
            float weight = spatialWeight * depthWeight;
            if (weight <= 0.0)
            {
                continue;
            }
            accumulated.atmosphere += sampleValue.atmosphere * weight;
            accumulated.metrics += sampleValue.metrics * weight;
            accumulated.aux += sampleValue.aux * weight;
            accumulated.temporal += sampleValue.temporal * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0.0001)
    {
        resolved.sample = sampleNearest(clamp(center, ivec2(0), ivec2(uHalfResolution) - ivec2(1)));
        resolved.edgeMask = 0.0;
        return resolved;
    }

    accumulated.atmosphere /= totalWeight;
    accumulated.metrics /= totalWeight;
    accumulated.aux /= totalWeight;
    accumulated.temporal /= totalWeight;
    resolved.sample = accumulated;
    resolved.edgeMask = clamp(totalWeight / max(totalSpatialWeight, 0.0001), 0.0, 1.0);
    return resolved;
}

void main()
{
    ResolvedVolumetricSample resolvedSample = resolveBilateral(vTexCoord);
    VolumetricSample sampleValue = resolvedSample.sample;
    vec4 nearestAtmosphere = texture(uHalfAtmosphereTexture, vTexCoord);

    if (uVolumetricDebugViewMode == 1)
    {
        FragColor = vec4(vec3(sampleValue.metrics.x), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 2)
    {
        FragColor = vec4(vec3(sampleValue.metrics.y), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 3)
    {
        FragColor = vec4(heatmapColor(sampleValue.metrics.z), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 4)
    {
        FragColor = vec4(heatmapColor(sampleValue.metrics.w), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 5)
    {
        FragColor = vec4(max(sampleValue.atmosphere.rgb, vec3(0.0)), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 6)
    {
        FragColor = vec4(max(sampleValue.aux.rgb, vec3(0.0)), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 7)
    {
        FragColor = vec4(max(nearestAtmosphere.rgb, vec3(0.0)), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 8)
    {
        vec2 cellUv = fract(vTexCoord * uHalfResolution);
        float grid = step(0.96, max(cellUv.x, cellUv.y));
        vec3 color = mix(max(nearestAtmosphere.rgb, vec3(0.0)), vec3(0.95, 0.42, 0.12), grid);
        FragColor = vec4(color, 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 9)
    {
        FragColor = vec4(heatmapColor(sampleValue.aux.a), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 10)
    {
        FragColor = vec4(heatmapColor(luminance(sampleValue.atmosphere.rgb)), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 11)
    {
        FragColor = vec4(heatmapColor(sampleValue.metrics.z * sampleValue.metrics.y), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 12)
    {
        float shadowContribution = clamp(luminance(sampleValue.aux.rgb) -
                                             luminance(sampleValue.atmosphere.rgb),
                                         0.0, 1.0);
        FragColor = vec4(heatmapColor(shadowContribution), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 13)
    {
        FragColor = vec4(heatmapColor(1.0 - sampleValue.metrics.y), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 14)
    {
        FragColor = vec4(vec3(sampleValue.temporal.z), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 15)
    {
        float rejected = sampleValue.temporal.z <= 0.0001 ? 1.0 : 0.0;
        FragColor = vec4(rejected, 0.0, 0.0, 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 16)
    {
        float velocityPixels = length((vTexCoord - sampleValue.temporal.xy) * uFullResolution);
        FragColor = vec4(heatmapColor(clamp(velocityPixels / 12.0, 0.0, 1.0)), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 17)
    {
        vec2 uv = clamp(sampleValue.temporal.xy, 0.0, 1.0);
        FragColor = vec4(uv, 0.0, 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 18)
    {
        FragColor = vec4(vec3(resolvedSample.edgeMask), 1.0);
        return;
    }

    if (uVolumetricDebugViewMode == 19)
    {
        FragColor = vec4(heatmapColor(sampleValue.temporal.w), 1.0);
        return;
    }

    FragColor = vec4(sampleValue.atmosphere.rgb, sampleValue.atmosphere.a);
}
