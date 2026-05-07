#version 330 core

in vec2 vTexCoord;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform sampler2D uSceneTexture;
uniform sampler2D uSceneDepthTexture;
uniform sampler2D uAtmosphereTexture;
uniform float uBloomThreshold;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec3 sceneColor = texture(uSceneTexture, vTexCoord).rgb;
    float sceneDepth = texture(uSceneDepthTexture, vTexCoord).r;
    vec4 atmosphere = texture(uAtmosphereTexture, vTexCoord);
    bool hitGeometry = sceneDepth < 0.999999;

    vec3 composedColor = hitGeometry ? sceneColor * atmosphere.a + atmosphere.rgb : atmosphere.rgb;
    float brightness = luminance(composedColor);
    vec3 bloomColor = brightness > uBloomThreshold
                          ? max(composedColor - vec3(uBloomThreshold), vec3(0.0))
                          : vec3(0.0);

    FragColor = vec4(max(composedColor, vec3(0.0)), 1.0);
    BrightColor = vec4(bloomColor, 1.0);
}
