#version 330 core

in vec2 vTexCoord;

layout (location = 0) out vec4 HistoryColor;
layout (location = 1) out vec4 MetricsColor;
layout (location = 2) out vec4 AuxColor;
layout (location = 3) out float DepthOut;
layout (location = 4) out vec4 TemporalDebugColor;

const int kMaxLocalLights = 8;
const int kMaxRayOccluders = 12;
const float kInvFourPi = 0.0795774715;

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
uniform sampler2D uPreviousHistoryTexture;
uniform sampler2D uPreviousDepthTexture;
uniform float uShadowStrength;
uniform float uScatteringStrength;
uniform float uVolumetricLightIntensity;
uniform float uDirectionalLightAngularRadius;
uniform float uStepLength;
uniform float uMaxDistance;
uniform int uMaxSteps;
uniform float uExtinctionStrength;
uniform float uAtmosphericAmbientFloor;
uniform float uTemporalBlend;
uniform float uTemporalDepthThreshold;
uniform float uTemporalNormalThreshold;
uniform float uTemporalVelocityThreshold;
uniform float uNearFieldHaze;
uniform float uPhaseAnisotropy;
uniform float uJitterStrength;
uniform float uStepDistributionExponent;
uniform float uTemporalJitterPhase;
uniform int uTemporalFrameIndex;
uniform vec2 uFullResolution;
uniform vec2 uHalfResolution;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogBaseHeight;
uniform float uFogHeightFalloff;
uniform float uFogMaxHeight;
uniform mat4 uInverseProjection;
uniform mat4 uPreviousInverseProjection;
uniform mat4 uInverseView;
uniform mat4 uPreviousViewProjection;
uniform vec3 uViewPosition;
uniform vec3 uPreviousViewPosition;
uniform vec3 uViewForward;
uniform vec3 uPreviousViewForward;
uniform vec3 uSunDirection;
uniform vec3 uPreviousLightDirection;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform vec3 uSkyHorizonColor;
uniform vec3 uSkyZenithColor;
uniform vec3 uGroundAmbientColor;
uniform float uSkyIntensity;
uniform float uSkyLightingEnabled;
uniform float uFogEnabled;
uniform float uHistoryValid;
uniform int uLocalLightCount;
uniform LocalLight uLocalLights[kMaxLocalLights];
uniform int uRayOccluderCount;
uniform RayOccluder uRayOccluders[kMaxRayOccluders];

float phaseForward(float cosTheta, float anisotropy)
{
    float g = clamp(anisotropy, -0.95, 0.95);
    float denominator = 1.0 + g * g - 2.0 * g * cosTheta;
    return kInvFourPi * (1.0 - g * g) / max(pow(denominator, 1.5), 0.001);
}

void buildOrthonormalBasis(vec3 direction, out vec3 tangent, out vec3 bitangent)
{
    vec3 referenceUp = abs(direction.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    tangent = normalize(cross(referenceUp, direction));
    bitangent = cross(direction, tangent);
}

float phaseForwardFiniteAngular(vec3 viewDirection, vec3 lightDirection, float anisotropy,
                                float angularRadius)
{
    float clampedRadius = clamp(angularRadius, 0.0, 0.05);
    float centerPhase =
        phaseForward(clamp(dot(viewDirection, lightDirection), -1.0, 1.0), anisotropy);
    if (clampedRadius <= 0.000001)
    {
        return centerPhase;
    }

    vec3 tangent;
    vec3 bitangent;
    buildOrthonormalBasis(lightDirection, tangent, bitangent);

    float accumulatedPhase = centerPhase;
    const vec2 offsets[4] = vec2[4](vec2(-0.5, -0.5), vec2(0.5, -0.5),
                                    vec2(-0.5, 0.5), vec2(0.5, 0.5));

    for (int index = 0; index < 4; ++index)
    {
        vec2 sampleOffset = offsets[index] * clampedRadius;
        vec3 perturbedDirection =
            normalize(lightDirection + tangent * sampleOffset.x + bitangent * sampleOffset.y);
        accumulatedPhase +=
            phaseForward(clamp(dot(viewDirection, perturbedDirection), -1.0, 1.0), anisotropy);
    }

    return accumulatedPhase * 0.2;
}

float interleavedGradientNoise(vec2 pixelCoord, float temporalPhase)
{
    float framePhase = floor(temporalPhase + 0.5);
    vec2 animatedPixel = pixelCoord + vec2(framePhase * 0.75487766, framePhase * 0.56984029);
    return fract(52.9829189 * fract(dot(animatedPixel, vec2(0.06711056, 0.00583715))));
}

float compressPositive(float value, float scale)
{
    return 1.0 - exp(-max(value, 0.0) * scale);
}

vec3 reconstructWorldRay(vec2 uv)
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 view = uInverseProjection * vec4(ndc, 1.0, 1.0);
    vec3 viewDirection = normalize(view.xyz / max(view.w, 0.0001));
    vec4 worldDirection = uInverseView * vec4(viewDirection, 0.0);
    return normalize(worldDirection.xyz);
}

vec3 reconstructViewRay(vec2 uv, mat4 inverseProjection)
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 view = inverseProjection * vec4(ndc, 1.0, 1.0);
    return view.xyz / max(view.w, 0.0001);
}

float linearDepth(vec2 uv, float deviceDepth)
{
    if (deviceDepth >= 0.999999)
    {
        return uMaxDistance;
    }

    vec4 clip = vec4(uv * 2.0 - 1.0, deviceDepth * 2.0 - 1.0, 1.0);
    vec4 view = uInverseProjection * clip;
    return max(-view.z / max(view.w, 0.0001), 0.0);
}

vec3 reconstructWorldPosition(vec2 uv, float deviceDepth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, deviceDepth * 2.0 - 1.0, 1.0);
    vec4 view = uInverseProjection * clip;
    view /= max(view.w, 0.0001);
    vec4 world = uInverseView * vec4(view.xyz, 1.0);
    return world.xyz;
}

vec3 reconstructViewPositionFromLinearDepth(vec2 uv, float linearDepth, mat4 inverseProjection)
{
    vec3 viewRay = reconstructViewRay(uv, inverseProjection);
    float scale = linearDepth / max(-viewRay.z, 0.0001);
    return viewRay * scale;
}

vec3 estimateSceneViewNormal(vec2 uv)
{
    vec2 texelSize = 1.0 / max(uFullResolution, vec2(1.0));
    float centerDeviceDepth = texture(uSceneDepthTexture, uv).r;
    if (centerDeviceDepth >= 0.999999)
    {
        return vec3(0.0, 0.0, 1.0);
    }

    float centerDepth = linearDepth(uv, centerDeviceDepth);
    vec3 centerPosition = reconstructViewPositionFromLinearDepth(uv, centerDepth, uInverseProjection);
    vec2 uvX = clamp(uv + vec2(texelSize.x, 0.0), vec2(0.0), vec2(1.0));
    vec2 uvY = clamp(uv + vec2(0.0, texelSize.y), vec2(0.0), vec2(1.0));
    float depthX = linearDepth(uvX, texture(uSceneDepthTexture, uvX).r);
    float depthY = linearDepth(uvY, texture(uSceneDepthTexture, uvY).r);
    vec3 positionX = reconstructViewPositionFromLinearDepth(uvX, depthX, uInverseProjection);
    vec3 positionY = reconstructViewPositionFromLinearDepth(uvY, depthY, uInverseProjection);
    return normalize(cross(positionX - centerPosition, positionY - centerPosition));
}

float samplePreviousLinearDepth(vec2 uv)
{
    return texture(uPreviousDepthTexture, uv).r;
}

vec3 estimatePreviousViewNormal(vec2 uv)
{
    vec2 texelSize = 1.0 / max(uHalfResolution, vec2(1.0));
    float centerDepth = samplePreviousLinearDepth(uv);
    if (centerDepth >= uMaxDistance)
    {
        return vec3(0.0, 0.0, 1.0);
    }

    vec3 centerPosition =
        reconstructViewPositionFromLinearDepth(uv, centerDepth, uPreviousInverseProjection);
    vec2 uvX = clamp(uv + vec2(texelSize.x, 0.0), vec2(0.0), vec2(1.0));
    vec2 uvY = clamp(uv + vec2(0.0, texelSize.y), vec2(0.0), vec2(1.0));
    float depthX = samplePreviousLinearDepth(uvX);
    float depthY = samplePreviousLinearDepth(uvY);
    vec3 positionX =
        reconstructViewPositionFromLinearDepth(uvX, depthX, uPreviousInverseProjection);
    vec3 positionY =
        reconstructViewPositionFromLinearDepth(uvY, depthY, uPreviousInverseProjection);
    return normalize(cross(positionX - centerPosition, positionY - centerPosition));
}

float distanceToFogCeiling(vec3 origin, vec3 direction)
{
    if (direction.y <= 0.0001)
    {
        return uMaxDistance;
    }

    float distance = (uFogMaxHeight - origin.y) / direction.y;
    return distance > 0.0 ? min(distance, uMaxDistance) : 0.0;
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
            visibility *= 0.12;
        }
    }

    return visibility;
}

vec2 reprojectUv(vec3 worldPosition)
{
    vec4 previousClip = uPreviousViewProjection * vec4(worldPosition, 1.0);
    if (previousClip.w <= 0.0)
    {
        return vec2(-1.0);
    }

    vec2 uv = previousClip.xy / previousClip.w * 0.5 + 0.5;
    return uv;
}

void main()
{
    float sceneDeviceDepth = texture(uSceneDepthTexture, vTexCoord).r;
    vec3 rayDirection = reconstructWorldRay(vTexCoord);
    float surfaceDistance = linearDepth(vTexCoord, sceneDeviceDepth);
    bool hitGeometry = sceneDeviceDepth < 0.999999;
    float jitterNoise = interleavedGradientNoise(vTexCoord * uFullResolution, uTemporalJitterPhase);
    float jitter = mix(0.5, jitterNoise, clamp(uJitterStrength, 0.0, 1.0));
    float marchLimit = min(surfaceDistance, distanceToFogCeiling(uViewPosition, rayDirection));
    float distributionExponent = max(uStepDistributionExponent, 0.35);

    float horizon = pow(1.0 - abs(dot(rayDirection, vec3(0.0, 1.0, 0.0))), 1.4);
    vec3 skyBase = mix(uSkyZenithColor, uSkyHorizonColor, clamp(horizon, 0.0, 1.0));
    vec3 baseAtmosphereColor = mix(uFogColor, skyBase, 0.35);
    vec3 sunLightDirection = normalize(-uSunDirection);
    float directionalPhase = phaseForwardFiniteAngular(rayDirection, sunLightDirection,
                                                       uPhaseAnisotropy,
                                                       uDirectionalLightAngularRadius);
    int targetSteps = clamp(int(ceil(marchLimit / max(uStepLength, 0.25))), 1, uMaxSteps);

    vec3 shadowedFog = vec3(0.0);
    vec3 unshadowedFog = vec3(0.0);
    float accumulatedTransmittance = 1.0;
    float accumulatedDensity = 0.0;
    float accumulatedScattering = 0.0;
    float accumulatedExtinction = 0.0;
    float integratedShadowing = 0.0;
    int stepsTaken = 0;

    for (int step = 0; step < targetSteps; ++step)
    {
        float progressStart = float(step) / float(max(targetSteps, 1));
        float progressEnd = float(step + 1) / float(max(targetSteps, 1));
        float segmentStart = marchLimit * pow(clamp(progressStart, 0.0, 1.0), distributionExponent);
        float segmentEnd = marchLimit * pow(clamp(progressEnd, 0.0, 1.0), distributionExponent);
        segmentEnd = min(segmentEnd, marchLimit);
        float segmentLength = max(segmentEnd - segmentStart, 0.0001);
        float sampleDistance = min(mix(segmentStart, segmentEnd, jitter), marchLimit);

        vec3 samplePosition = uViewPosition + rayDirection * sampleDistance;
        if (samplePosition.y > uFogMaxHeight)
        {
            break;
        }

        float heightDensity = exp(-max(samplePosition.y - uFogBaseHeight, 0.0) * uFogHeightFalloff);
        float distanceHaze = 1.0 + max(uNearFieldHaze, 0.0) * exp(-sampleDistance * 0.045);
        float density = uFogDensity * heightDensity * distanceHaze;
        float scatteringCoefficient = density * max(uScatteringStrength, 0.0);
        float extinctionCoefficient = max(density * max(uExtinctionStrength, 0.0), 0.00001);
        float segmentTransmittance = exp(-extinctionCoefficient * segmentLength);
        float skyScatter = clamp(uSkyIntensity * uSkyLightingEnabled, 0.0, 1.0);
        float rawSunVisibility = directionalVisibility(samplePosition, sunLightDirection);
        float sunVisibility = mix(1.0, rawSunVisibility, clamp(uShadowStrength, 0.0, 1.0));

        float ambientScatter = max(uAtmosphericAmbientFloor, 0.0) +
                               skyScatter * kInvFourPi * (0.22 + horizon * 0.16);
        vec3 ambientContribution = baseAtmosphereColor * scatteringCoefficient * ambientScatter;
        vec3 directLightBase = uSunColor * uSunIntensity * scatteringCoefficient * directionalPhase *
                               max(uVolumetricLightIntensity, 0.0) * 0.08;
        vec3 localLightContribution = vec3(0.0);

        for (int index = 0; index < uLocalLightCount && index < kMaxLocalLights; ++index)
        {
            vec3 toLight = uLocalLights[index].position - samplePosition;
            float lightDistance = length(toLight);
            float lightRange = max(uLocalLights[index].range, 0.001);
            if (lightDistance >= lightRange)
            {
                continue;
            }

            vec3 lightDirection = toLight / max(lightDistance, 0.001);
            float rangeFade = 1.0 - clamp(lightDistance / lightRange, 0.0, 1.0);
            float attenuation = (rangeFade * rangeFade) / (1.0 + lightDistance * lightDistance * 0.35);
            float localPhase = phaseForward(clamp(dot(rayDirection, lightDirection), -1.0, 1.0), 0.15);
            localLightContribution += uLocalLights[index].color * uLocalLights[index].intensity *
                                      scatteringCoefficient * attenuation * localPhase * 0.08;
        }

        vec3 shadowedLightContribution = directLightBase * sunVisibility + localLightContribution;
        vec3 unshadowedLightContribution = directLightBase + localLightContribution;
        vec3 shadowedScattering = ambientContribution + shadowedLightContribution;
        vec3 unshadowedScattering = ambientContribution + unshadowedLightContribution;

        vec3 shadowedStep = shadowedScattering * (1.0 - segmentTransmittance) /
                            max(extinctionCoefficient, 0.00001);
        vec3 unshadowedStep = unshadowedScattering * (1.0 - segmentTransmittance) /
                              max(extinctionCoefficient, 0.00001);
        shadowedFog += shadowedStep * accumulatedTransmittance;
        unshadowedFog += unshadowedStep * accumulatedTransmittance;
        accumulatedTransmittance *= segmentTransmittance;
        accumulatedDensity += density * segmentLength;
        accumulatedScattering += length(shadowedScattering) * segmentLength;
        accumulatedExtinction += extinctionCoefficient * segmentLength;
        integratedShadowing += (1.0 - sunVisibility) * density * segmentLength;
        stepsTaken += 1;

        if (accumulatedTransmittance <= 0.003)
        {
            break;
        }
    }

    float densityMetric = compressPositive(accumulatedDensity, 1.2);
    float transmittanceMetric = clamp(accumulatedTransmittance, 0.0, 1.0);
    float scatteringMetric = compressPositive(accumulatedScattering, 0.7);
    float stepsMetric = float(stepsTaken) / float(max(targetSteps, 1));
    float extinctionMetric = clamp(1.0 - exp(-accumulatedExtinction), 0.0, 1.0);
    vec3 skyColor = mix(uSkyZenithColor, uSkyHorizonColor, clamp(horizon * 0.92, 0.0, 1.0));
    skyColor = mix(skyColor, uGroundAmbientColor, pow(clamp(1.0 - rayDirection.y, 0.0, 1.0), 3.0) * 0.24);
    skyColor *= clamp(uSkyIntensity * uSkyLightingEnabled, 0.0, 1.0);

    shadowedFog *= uFogEnabled;
    unshadowedFog *= uFogEnabled;
    vec3 skyThroughAtmosphere = skyColor * transmittanceMetric + shadowedFog;
    vec4 currentAtmosphere = hitGeometry ? vec4(shadowedFog, transmittanceMetric)
                                         : vec4(skyThroughAtmosphere, 0.0);

    vec3 reprojectionWorldPosition = hitGeometry
                                         ? reconstructWorldPosition(vTexCoord, sceneDeviceDepth)
                                         : uViewPosition + rayDirection * marchLimit;
    vec2 previousUv = reprojectUv(reprojectionWorldPosition);
    float historyBlend = 0.0;
    float historyConfidence = 0.0;
    vec4 previousHistory = vec4(0.0);
    if (uHistoryValid > 0.5 && all(greaterThanEqual(previousUv, vec2(0.0))) &&
        all(lessThanEqual(previousUv, vec2(1.0))))
    {
        previousHistory = texture(uPreviousHistoryTexture, previousUv);
        float previousDepth = samplePreviousLinearDepth(previousUv);
        bool previousHitGeometry = previousDepth < uMaxDistance * 0.999;
        float depthThreshold = max(uTemporalDepthThreshold,
                       max(surfaceDistance, previousDepth) * 0.01);
        float depthDifference = abs(previousDepth - surfaceDistance);
        bool depthRejected = hitGeometry != previousHitGeometry ||
                             (hitGeometry && depthDifference > depthThreshold);

        vec3 currentNormal = hitGeometry ? estimateSceneViewNormal(vTexCoord) : vec3(0.0, 0.0, 1.0);
        vec3 previousNormal = previousHitGeometry ? estimatePreviousViewNormal(previousUv)
                                                  : vec3(0.0, 0.0, 1.0);
        float normalSimilarity = clamp(dot(currentNormal, previousNormal), 0.0, 1.0);
        bool normalRejected = hitGeometry && previousHitGeometry &&
                              normalSimilarity < uTemporalNormalThreshold;

        float velocityPixels = length((vTexCoord - previousUv) * uFullResolution);
        bool velocityRejected = velocityPixels > uTemporalVelocityThreshold;

        float cameraRotation = clamp(dot(normalize(uViewForward), normalize(uPreviousViewForward)),
                                     0.0, 1.0);
        float lightRotation = clamp(dot(normalize(-uSunDirection), normalize(-uPreviousLightDirection)),
                                    0.0, 1.0);
        float cameraTranslation = length(uViewPosition - uPreviousViewPosition);
        float cameraMotionConfidence =
            (1.0 - smoothstep(0.05, 0.85, cameraTranslation)) *
            smoothstep(0.94, 0.9995, cameraRotation);
        float lightMotionConfidence = smoothstep(0.96, 0.9998, lightRotation);
        float velocityConfidence = 1.0 - smoothstep(1.0, uTemporalVelocityThreshold, velocityPixels);
        float depthConfidence = 1.0 - smoothstep(depthThreshold * 0.5, depthThreshold, depthDifference);
        float normalConfidence = hitGeometry && previousHitGeometry
                                     ? smoothstep(uTemporalNormalThreshold, 1.0, normalSimilarity)
                                     : 1.0;

        bool rejectHistory = depthRejected || normalRejected || velocityRejected;
        historyConfidence = rejectHistory
                                ? 0.0
                                : clamp(depthConfidence * normalConfidence * velocityConfidence *
                                            cameraMotionConfidence * lightMotionConfidence,
                                        0.0, 1.0);
        historyBlend = clamp(uTemporalBlend, 0.0, 0.97) * historyConfidence;
        float historyDifference = length(previousHistory.rgb - currentAtmosphere.rgb) * 0.5 +
                                  abs(previousHistory.a - currentAtmosphere.a);
        historyBlend *= clamp(1.0 - historyDifference * 1.5, 0.0, 1.0);
        historyConfidence *= clamp(1.0 - historyDifference * 1.25, 0.0, 1.0);
    }

    HistoryColor = mix(currentAtmosphere, previousHistory, historyBlend);
    MetricsColor = vec4(densityMetric, transmittanceMetric, scatteringMetric, stepsMetric);
    AuxColor = vec4(unshadowedFog, extinctionMetric);
    DepthOut = hitGeometry ? surfaceDistance : uMaxDistance;
    TemporalDebugColor = vec4(previousUv, historyBlend, historyConfidence);
}
