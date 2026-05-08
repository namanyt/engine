#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uSceneTexture;
uniform sampler2D uBloomTexture;
uniform float uExposure;
uniform float uBloomIntensity;
uniform float uGamma;
uniform float uContrast;
uniform float uVignetteStrength;
uniform float uSaturation;
uniform float uMidtoneLift;
uniform float uToneMappingEnabled;
uniform float uPostDebugViewMode;
uniform float uDebugExposureScale;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 acesApproximation(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 tonemapAces(vec3 color)
{
    return clamp(acesApproximation(color), 0.0, 1.0);
}

vec3 applyGamma(vec3 color, float gammaValue)
{
    return pow(max(color, vec3(0.0)), vec3(1.0 / max(gammaValue, 0.0001)));
}

vec3 encodeHdrDebug(vec3 color)
{
    return applyGamma(color / (1.0 + color), uGamma);
}

vec3 encodeLuminanceDebug(float value)
{
    float compressed = clamp(log2(1.0 + value * 16.0) / 4.0, 0.0, 1.0);
    return vec3(compressed);
}

void main()
{
    vec3 sceneColor = texture(uSceneTexture, vTexCoord).rgb;
    vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;

    if (uPostDebugViewMode > 0.5)
    {
        int debugMode = int(uPostDebugViewMode + 0.5);
        vec3 exposureColor = sceneColor * uDebugExposureScale;
        vec3 toneMappedColor = tonemapAces(exposureColor + bloomColor * uBloomIntensity);

        if (debugMode == 1)
        {
            FragColor = vec4(encodeHdrDebug(sceneColor), 1.0);
            return;
        }

        if (debugMode == 2)
        {
            FragColor = vec4(encodeLuminanceDebug(luminance(sceneColor)), 1.0);
            return;
        }

        if (debugMode == 3)
        {
            FragColor = vec4(encodeHdrDebug(bloomColor), 1.0);
            return;
        }

        if (debugMode == 4)
        {
            FragColor = vec4(encodeHdrDebug(exposureColor), 1.0);
            return;
        }

        if (debugMode == 5)
        {
            FragColor = vec4(applyGamma(toneMappedColor, uGamma), 1.0);
            return;
        }
    }

    if (uToneMappingEnabled < 0.5)
    {
        FragColor = vec4(applyGamma(max(sceneColor, vec3(0.0)), uGamma), 1.0);
        return;
    }

    vec3 hdrColor = sceneColor + bloomColor * uBloomIntensity;
    hdrColor *= uExposure;
    hdrColor = max(hdrColor, vec3(0.0));

    vec3 mapped = tonemapAces(hdrColor);
    mapped = pow(mapped, vec3(1.0 / max(uContrast, 0.0001)));
    float mappedLuminance = luminance(mapped);
    mapped = mix(vec3(mappedLuminance), mapped, uSaturation);
    mapped += vec3(uMidtoneLift) * clamp(mappedLuminance, 0.0, 1.0) * (1.0 - mappedLuminance);

    vec2 centeredUv = vTexCoord * 2.0 - 1.0;
    float vignette = 1.0 - dot(centeredUv, centeredUv) * uVignetteStrength;
    mapped *= clamp(vignette, 0.0, 1.0);
    mapped = applyGamma(mapped, uGamma);

    FragColor = vec4(mapped, 1.0);
}
