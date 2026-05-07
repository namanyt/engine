#version 330 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec4 vColor;
in vec4 vShadowPosition;

const int kMaxLocalLights = 8;

struct LocalLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

uniform vec3 uViewPosition;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform vec3 uSkyHorizonColor;
uniform vec3 uSkyZenithColor;
uniform vec3 uGroundAmbientColor;
uniform float uSkyIntensity;
uniform float uAmbientEnabled;
uniform float uSkyLightingEnabled;
uniform float uEmissiveEnabled;
uniform int uMaterialDebugViewMode;
uniform int uMaterialCategory;
uniform vec3 uMaterialAlbedo;
uniform vec3 uMaterialEmissiveColor;
uniform float uMaterialEmissiveStrength;
uniform float uMaterialRoughness;
uniform float uMaterialMetallic;
uniform float uMaterialSpecularStrength;
uniform float uMaterialSoftness;
uniform float uMaterialAtmosphericResponse;
uniform sampler2D uShadowMap;
uniform float uShadowBias;
uniform float uShadowNormalBias;
uniform float uShadowStrength;
uniform int uLocalLightCount;
uniform LocalLight uLocalLights[kMaxLocalLights];
uniform float uTime;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 LightColor;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float compressPositive(float value, float scale)
{
    return 1.0 - exp(-max(value, 0.0) * scale);
}

vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
    float factor = pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return f0 + (vec3(1.0) - f0) * factor;
}

vec3 heatmapColor(float value)
{
    float clamped = clamp(value, 0.0, 1.0);
    return clamp(vec3(1.5 * clamped,
                      1.4 * (1.0 - abs(clamped - 0.5) * 2.0),
                      1.35 * (1.0 - clamped)),
                 0.0,
                 1.0);
}

vec3 materialIdColor(int category)
{
    if (category == 1)
    {
        return vec3(0.54, 0.48, 0.41);
    }

    if (category == 2)
    {
        return vec3(0.18, 0.46, 0.62);
    }

    if (category == 3)
    {
        return vec3(0.62, 0.72, 0.80);
    }

    if (category == 4)
    {
        return vec3(1.00, 0.68, 0.26);
    }

    if (category == 5)
    {
        return vec3(0.16, 0.12, 0.14);
    }

    if (category == 6)
    {
        return vec3(0.56, 0.76, 0.68);
    }

    return vec3(1.0, 0.0, 1.0);
}

float computeShadow(vec3 normal, vec3 lightDirection)
{
    vec3 projected = vShadowPosition.xyz / vShadowPosition.w;
    projected = projected * 0.5 + 0.5;

    if (projected.z > 1.0 || projected.x < 0.0 || projected.x > 1.0 || projected.y < 0.0 ||
        projected.y > 1.0)
    {
        return 0.0;
    }

    float bias = max(uShadowBias, uShadowNormalBias * (1.0 - max(dot(normal, lightDirection), 0.0)));
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(uShadowMap, projected.xy + vec2(x, y) * texelSize).r;
            shadow += projected.z > closestDepth + bias ? 1.0 : 0.0;
        }
    }

    return (shadow / 9.0) * uShadowStrength;
}

void main()
{
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDirection = normalize(-uSunDirection);
    vec3 viewDirection = normalize(uViewPosition - vWorldPosition);
    vec3 halfVector = normalize(lightDirection + viewDirection);
    float shadow = computeShadow(normal, lightDirection);
    float roughness = clamp(uMaterialRoughness, 0.04, 1.0);
    float metallic = clamp(uMaterialMetallic, 0.0, 1.0);
    float softness = clamp(uMaterialSoftness, 0.0, 1.0);
    float atmosphereResponse = max(uMaterialAtmosphericResponse, 0.0);

    vec3 vertexColor = mix(vec3(1.0), pow(max(vColor.rgb, vec3(0.0)), vec3(1.08)), 0.28);
    vec3 baseColor = uMaterialAlbedo * vertexColor;
    baseColor *= 0.988 + 0.012 * vec3(vTexCoord, 1.0);
    baseColor *= 0.996 + 0.004 * sin(uTime * 0.05 + vWorldPosition.x * 0.018 + vWorldPosition.z * 0.013);

    float upFactor = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyAmbient = mix(uGroundAmbientColor, uSkyHorizonColor, upFactor);
    skyAmbient = mix(skyAmbient, uSkyZenithColor, pow(clamp(normal.y, 0.0, 1.0), 1.6));

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float wrappedDiffuse = clamp((dot(normal, lightDirection) + softness * 0.42) /
                                     (1.0 + softness * 0.42),
                                 0.0,
                                 1.0);
    wrappedDiffuse = smoothstep(0.0, 1.0, wrappedDiffuse);
    float shadowedSun = 1.0 - shadow * 0.82;
    float specularPower = mix(96.0, 10.0, roughness);
    vec3 f0 = mix(vec3(0.024 + uMaterialSpecularStrength * 0.18), baseColor, metallic);
    vec3 fresnel = fresnelSchlick(max(dot(halfVector, viewDirection), 0.0), f0);
    float specularLobe = pow(max(dot(normal, halfVector), 0.0), specularPower);
    vec3 sunSpecular = fresnel * specularLobe * uMaterialSpecularStrength * shadowedSun;
    float softBackScatter = pow(1.0 - diffuse, 2.2) * softness;
    float edgeFactor = pow(1.0 - max(dot(normal, viewDirection), 0.0), mix(3.6, 1.45, atmosphereResponse));
    vec3 edgeLighting = uSunColor * edgeFactor * atmosphereResponse * (0.05 + wrappedDiffuse * 0.16) * shadowedSun;

    vec3 ambient = skyAmbient * uSkyIntensity * uAmbientEnabled * uSkyLightingEnabled;
    vec3 color = baseColor * ambient;
    color += baseColor * (1.0 - metallic * 0.72) * uSunColor * wrappedDiffuse * uSunIntensity * shadowedSun;
    color += uSunColor * sunSpecular * mix(0.18, 0.80, metallic + (1.0 - roughness) * 0.45);
    color += baseColor * uSunColor * softBackScatter * uSunIntensity * 0.12;
    color += edgeLighting;

    vec3 atmosphereLightField =
        (baseColor * wrappedDiffuse * uSunColor * uSunIntensity * 0.05 +
         uSunColor * sunSpecular * (0.48 + atmosphereResponse * 0.72) + edgeLighting * 2.1) *
        atmosphereResponse;
    vec3 localSpecularAccumulation = vec3(0.0);

    for (int index = 0; index < uLocalLightCount && index < kMaxLocalLights; ++index)
    {
        vec3 lightVector = uLocalLights[index].position - vWorldPosition;
        float distanceToLight = length(lightVector);
        vec3 localDirection = distanceToLight > 0.0001 ? lightVector / distanceToLight : vec3(0.0, 1.0, 0.0);
        float attenuation = clamp(1.0 - distanceToLight / uLocalLights[index].range, 0.0, 1.0);
        attenuation *= attenuation;

        float localDiffuse = clamp((dot(normal, localDirection) + softness * 0.35) /
                                       (1.0 + softness * 0.35),
                                   0.0,
                                   1.0);
        vec3 localHalfVector = normalize(localDirection + viewDirection);
        float localSpecular = pow(max(dot(normal, localHalfVector), 0.0), mix(84.0, 9.0, roughness));
        vec3 localColor = uLocalLights[index].color * uLocalLights[index].intensity * attenuation;
        vec3 localFresnel = fresnelSchlick(max(dot(localHalfVector, viewDirection), 0.0), f0);
        vec3 localSpecularColor = localColor * localSpecular * uMaterialSpecularStrength * localFresnel;

        color += mix(baseColor * 0.22, baseColor, 0.78 - metallic * 0.42) * localColor * localDiffuse;
        color += localSpecularColor * mix(0.22, 1.15, metallic + (1.0 - roughness) * 0.38);
        color += localColor * baseColor * softness * pow(1.0 - localDiffuse, 2.0) * 0.08;

        localSpecularAccumulation += localSpecularColor;
        atmosphereLightField +=
            (localColor * (0.16 + localDiffuse * 0.18) + localSpecularColor * 1.45) * atmosphereResponse;
    }

    vec3 emissive = uMaterialEmissiveColor * uMaterialEmissiveStrength * uEmissiveEnabled;
    color += emissive;

    vec3 surfaceLightField = atmosphereLightField + emissive * (0.45 + atmosphereResponse * 1.15);

    if (uMaterialDebugViewMode > 0)
    {
        vec3 debugColor = vec3(0.0);

        if (uMaterialDebugViewMode == 1)
        {
            debugColor = materialIdColor(uMaterialCategory);
        }
        else if (uMaterialDebugViewMode == 2)
        {
            debugColor = vec3(1.0 - roughness);
        }
        else if (uMaterialDebugViewMode == 3)
        {
            float specularMetric = compressPositive(luminance(sunSpecular + localSpecularAccumulation), 2.8);
            debugColor = vec3(specularMetric);
        }
        else if (uMaterialDebugViewMode == 4)
        {
            debugColor = emissive;
        }
        else if (uMaterialDebugViewMode == 5)
        {
            float atmosphereMetric = compressPositive(luminance(surfaceLightField), 0.85);
            debugColor = mix(vec3(atmosphereMetric), materialIdColor(uMaterialCategory), 0.28);
        }
        else if (uMaterialDebugViewMode == 6)
        {
            debugColor = heatmapColor(compressPositive(luminance(color), 0.55));
        }

        FragColor = vec4(max(debugColor, vec3(0.0)), 1.0);
        LightColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    FragColor = vec4(color, 1.0);
    LightColor = vec4(max(surfaceLightField, vec3(0.0)), 1.0);
}
