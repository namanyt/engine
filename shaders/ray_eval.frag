#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

const int kMaxLocalLights = 8;
const int kMaxRayOccluders = 12;

struct LocalLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

struct RayOccluder
{
    vec3 center;
    float radius;
};

uniform sampler2D uSceneDepthTexture;
uniform sampler2D uSurfaceLightingTexture;
uniform float uShadowStrength;
uniform float uAtmosphereIntensity;
uniform float uEmissiveScatter;
uniform float uStepLength;
uniform float uMaxDistance;
uniform int uMaxSteps;
uniform float uExtinction;
uniform float uNearFieldHaze;
uniform float uPhaseAnisotropy;
uniform float uJitterStrength;
uniform float uStepDistributionExponent;
uniform float uTemporalJitterPhase;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogBaseHeight;
uniform float uFogHeightFalloff;
uniform vec3 uViewPosition;
uniform vec3 uViewForward;
uniform vec3 uViewRight;
uniform vec3 uViewUp;
uniform float uAspectRatio;
uniform float uVerticalFieldOfViewRadians;
uniform float uNearPlane;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform vec3 uSkyHorizonColor;
uniform vec3 uSkyZenithColor;
uniform vec3 uGroundAmbientColor;
uniform float uSkyIntensity;
uniform float uSkyLightingEnabled;
uniform float uFogEnabled;
uniform int uLocalLightCount;
uniform LocalLight uLocalLights[kMaxLocalLights];
uniform int uRayOccluderCount;
uniform RayOccluder uRayOccluders[kMaxRayOccluders];
uniform int uVolumetricDebugViewMode;

float phaseForward(float cosTheta, float anisotropy)
{
    float g = clamp(anisotropy, -0.95, 0.95);
    float denominator = 1.0 + g * g - 2.0 * g * cosTheta;
    return (1.0 - g * g) / max(pow(denominator, 1.5), 0.001);
}

float interleavedGradientNoise(vec2 pixelCoord, float temporalPhase)
{
    vec2 animatedPixel = pixelCoord + vec2(temporalPhase * 0.75487766, temporalPhase * 0.56984029);
    return fract(52.9829189 * fract(dot(animatedPixel, vec2(0.06711056, 0.00583715))));
}

float marchDistance(float progress, float rayEndDistance, float exponent)
{
    return rayEndDistance * pow(clamp(progress, 0.0, 1.0), max(exponent, 0.35));
}

vec3 heatmapColor(float value)
{
    float clamped = clamp(value, 0.0, 1.0);
    return clamp(vec3(1.5 * clamped,
                      1.5 * (1.0 - abs(clamped - 0.5) * 2.0),
                      1.5 * (1.0 - clamped)),
                 0.0, 1.0);
}

float compressPositive(float value, float scale)
{
    return 1.0 - exp(-max(value, 0.0) * scale);
}

vec3 viewRayDirection(vec2 uv)
{
    float tanHalfFov = tan(uVerticalFieldOfViewRadians * 0.5);
    vec2 ndc = uv * 2.0 - 1.0;
    vec3 ray = normalize(uViewForward + uViewRight * ndc.x * tanHalfFov * uAspectRatio +
                         uViewUp * ndc.y * tanHalfFov);
    return ray;
}

float sceneDepthDistance(vec2 uv, vec3 rayDirection)
{
    float deviceDepth = texture(uSceneDepthTexture, uv).r;
    if (deviceDepth >= 0.999999)
    {
        return uMaxDistance;
    }

    float viewSpaceDepth = uNearPlane / max(1.0 - deviceDepth, 0.0001);
    float viewAlignment = max(dot(rayDirection, uViewForward), 0.0001);
    return min(viewSpaceDepth / viewAlignment, uMaxDistance);
}

float intersectSphere(vec3 origin, vec3 direction, vec3 center, float radius)
{
    vec3 relativeOrigin = origin - center;
    float b = dot(relativeOrigin, direction);
    float c = dot(relativeOrigin, relativeOrigin) - radius * radius;
    float discriminant = b * b - c;

    if (discriminant < 0.0)
    {
        return -1.0;
    }

    float distanceNear = -b - sqrt(discriminant);
    if (distanceNear > 0.0)
    {
        return distanceNear;
    }

    float distanceFar = -b + sqrt(discriminant);
    return distanceFar > 0.0 ? distanceFar : -1.0;
}

float directionalVisibility(vec3 samplePosition, vec3 lightDirection)
{
    float visibility = 1.0;

    for (int index = 0; index < uRayOccluderCount && index < kMaxRayOccluders; ++index)
    {
        float hitDistance = intersectSphere(samplePosition + lightDirection * 0.25, lightDirection,
                                            uRayOccluders[index].center,
                                            uRayOccluders[index].radius);
        if (hitDistance > 0.0 && hitDistance < uMaxDistance)
        {
            visibility *= 0.22;
        }
    }

    return visibility;
}

void main()
{
    vec3 surfaceLighting = texture(uSurfaceLightingTexture, vTexCoord).rgb;
    vec3 rayDirection = viewRayDirection(vTexCoord);
    float rayEndDistance = max(sceneDepthDistance(vTexCoord, rayDirection), 0.0001);
    bool hitGeometry = rayEndDistance < (uMaxDistance - uStepLength);
    float jitterNoise = interleavedGradientNoise(gl_FragCoord.xy, uTemporalJitterPhase);
    float jitter = mix(0.5, jitterNoise, clamp(uJitterStrength, 0.0, 1.0));
    float marchLimit = min(rayEndDistance, uMaxDistance);
    float distributionExponent = max(uStepDistributionExponent, 0.35);

    float horizon = pow(1.0 - abs(dot(rayDirection, vec3(0.0, 1.0, 0.0))), 1.4);
    vec3 rayFog = vec3(0.0);
    float opticalDepth = 0.0;
    float accumulatedDensity = 0.0;
    float accumulatedSegmentLength = 0.0;
    float maxSegmentLength = 0.0;
    float firstSegmentLength = 0.0;
    float firstSampleDistance = 0.0;
    float integratedShadowing = 0.0;
    int stepsTaken = 0;
    vec3 skyBase = mix(uSkyZenithColor, uSkyHorizonColor, clamp(horizon, 0.0, 1.0));
    vec3 baseAtmosphereColor = mix(uFogColor, skyBase, 0.55);
    float viewLightAlignment = clamp(dot(rayDirection, normalize(-uSunDirection)), -1.0, 1.0);
    float directionalPhase = phaseForward(viewLightAlignment, uPhaseAnisotropy);

    for (int step = 0; step < uMaxSteps; ++step)
    {
        float progressStart = float(step) / float(max(uMaxSteps, 1));
        float progressEnd = float(step + 1) / float(max(uMaxSteps, 1));
        float segmentStart = marchDistance(progressStart, marchLimit, distributionExponent);
        if (segmentStart >= marchLimit)
        {
            break;
        }

        float segmentEnd = marchDistance(progressEnd, marchLimit, distributionExponent);
        segmentEnd = min(segmentEnd, marchLimit);
        float segmentLength = max(segmentEnd - segmentStart, 0.0001);
        float sampleDistance = min(mix(segmentStart, segmentEnd, jitter), marchLimit);

        if (step == 0)
        {
            firstSegmentLength = segmentLength;
            firstSampleDistance = sampleDistance;
        }

        vec3 samplePosition = uViewPosition + rayDirection * sampleDistance;
        float sampleHeight = samplePosition.y;
        float heightDensity = exp(-(sampleHeight - uFogBaseHeight) * uFogHeightFalloff);
        heightDensity = clamp(heightDensity, 0.0, 3.5);
        float horizonBoost = 1.0 + horizon * 2.9;
        float nearFieldDensity = 1.0 + uNearFieldHaze * exp(-sampleDistance * 0.024);
        float density = uFogDensity * uAtmosphereIntensity * heightDensity * horizonBoost * nearFieldDensity;
        float transmittance = exp(-opticalDepth * (0.85 + uExtinction));
        float sunVisibility = directionalVisibility(samplePosition, normalize(-uSunDirection));
        float skyScatter = clamp(uSkyIntensity * uSkyLightingEnabled, 0.0, 1.0);
        vec3 stepScatter = baseAtmosphereColor * density * (0.08 + skyScatter * 0.18);
        stepScatter += uSunColor * uSunIntensity * sunVisibility * density * directionalPhase * 0.092;
        float densityIntegral = density * segmentLength * 0.025;
        rayFog += stepScatter * transmittance * segmentLength * 0.025;
        opticalDepth += densityIntegral;
        accumulatedDensity += densityIntegral;
        accumulatedSegmentLength += segmentLength;
        maxSegmentLength = max(maxSegmentLength, segmentLength);
        integratedShadowing += (1.0 - sunVisibility) * densityIntegral;
        stepsTaken += 1;
    }

    float normalizedSampleCount = float(stepsTaken) / float(max(uMaxSteps, 1));
    float averageSegmentLength = stepsTaken > 0 ? accumulatedSegmentLength / float(stepsTaken) : 0.0;
    float expectedLinearStepLength = marchLimit / float(max(uMaxSteps, 1));
    float normalizedStepMetric = clamp(averageSegmentLength / max(expectedLinearStepLength, 0.0001), 0.0, 2.0);
    float firstSegmentMetric = clamp(firstSegmentLength / max(expectedLinearStepLength, 0.0001), 0.0, 2.0);
    float depthMetric = clamp(marchLimit / max(uMaxDistance, 0.0001), 0.0, 1.0);
    float jitterMetric = clamp(firstSampleDistance / max(firstSegmentLength, 0.0001), 0.0, 1.0);
    float densityMetric = compressPositive(accumulatedDensity, 1.2);
    float shadowMetric = compressPositive(integratedShadowing, 6.0);

    vec3 localScatter = vec3(0.0);
    float activeLightEnergy = 0.0;
    for (int index = 0; index < uLocalLightCount && index < kMaxLocalLights; ++index)
    {
        vec3 lightVector = uLocalLights[index].position - uViewPosition;
        float projectedDistance = max(dot(lightVector, rayDirection), 0.0);
        float lateralDistance = length(lightVector - rayDirection * projectedDistance);
        float range = max(uLocalLights[index].range, 0.001);
        float influence = exp(-lateralDistance / (range * 0.32)) * exp(-projectedDistance / (range * 0.75));
        influence *= uLocalLights[index].intensity;
        localScatter += uLocalLights[index].color * influence * (0.008 + uAtmosphereIntensity * 0.012);
        activeLightEnergy += uLocalLights[index].intensity;
    }

    float surfaceLightEnergy = dot(surfaceLighting, vec3(0.2126, 0.7152, 0.0722));
    float lightPresence = clamp(activeLightEnergy * 0.08 + surfaceLightEnergy * 0.32, 0.0, 1.0);
    float mediumTransmittance = exp(-opticalDepth * (0.65 + uExtinction));
    float transmittanceMetric = clamp(1.0 - mediumTransmittance, 0.0, 1.0);
    vec3 emissiveFog = surfaceLighting * uEmissiveScatter * (0.04 + horizon * 0.05);
    vec3 skyColor = mix(uSkyZenithColor, uSkyHorizonColor, clamp(horizon * 0.92, 0.0, 1.0));
    skyColor = mix(skyColor, uGroundAmbientColor, pow(clamp(1.0 - rayDirection.y, 0.0, 1.0), 3.0) * 0.24);
    skyColor *= clamp(uSkyIntensity * uSkyLightingEnabled, 0.0, 1.0);

    vec3 worldAtmosphere = rayFog * (0.18 + lightPresence * 1.24) + localScatter * 0.82 + emissiveFog * 0.72;
    worldAtmosphere *= uFogEnabled;
    vec3 skyThroughAtmosphere = skyColor * mediumTransmittance + worldAtmosphere;

    if (uVolumetricDebugViewMode == 1)
    {
        vec3 debugColor = vec3(depthMetric,
                               clamp(firstSegmentMetric * 0.5, 0.0, 1.0),
                               clamp(abs(jitterMetric - 0.5) * 2.0, 0.0, 1.0));
        FragColor = vec4(debugColor, 0.0);
        return;
    }

    if (uVolumetricDebugViewMode == 2)
    {
        vec3 debugColor = vec3(densityMetric);
        FragColor = vec4(debugColor, 0.0);
        return;
    }

    if (uVolumetricDebugViewMode == 3)
    {
        FragColor = vec4(max(worldAtmosphere, vec3(0.0)), 0.0);
        return;
    }

    if (uVolumetricDebugViewMode == 4)
    {
        float heat = clamp(densityMetric * 0.45 + shadowMetric * 0.40 + transmittanceMetric * 0.15,
                           0.0,
                           1.0);
        FragColor = vec4(heatmapColor(heat), 0.0);
        return;
    }

    if (uVolumetricDebugViewMode == 5)
    {
        FragColor = vec4(vec3(normalizedSampleCount), 0.0);
        return;
    }

    if (hitGeometry)
    {
        FragColor = vec4(worldAtmosphere, mediumTransmittance);
        return;
    }

    FragColor = vec4(skyThroughAtmosphere, 0.0);
}
