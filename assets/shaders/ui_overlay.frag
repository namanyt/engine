#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uOverlayTexture;
uniform sampler2D uBackgroundTexture;
uniform vec2 uScreenSize;
uniform vec2 uOverlaySizePixels;
uniform vec2 uOverlayMinPixels;
uniform int uOverlayEffect;
uniform float uOverlayOpacity;

void main()
{
    vec2 fragmentPosition = vTexCoord * uScreenSize;
    vec2 overlayMin = uOverlayMinPixels;
    vec2 overlayMax = overlayMin + uOverlaySizePixels;

    if (fragmentPosition.x < overlayMin.x || fragmentPosition.x > overlayMax.x ||
        fragmentPosition.y < overlayMin.y || fragmentPosition.y > overlayMax.y)
    {
        discard;
    }

    vec2 overlayUv = (fragmentPosition - overlayMin) / uOverlaySizePixels;
    vec4 color = texture(uOverlayTexture, vec2(overlayUv.x, 1.0 - overlayUv.y));
    if (color.a <= 0.001)
    {
        discard;
    }

    if (uOverlayEffect == 1)
    {
        vec3 background = texture(uBackgroundTexture, vTexCoord).rgb;
        float mask = smoothstep(0.02, 0.92, color.a);
        float halo = smoothstep(0.0, 0.36, color.a) * (1.0 - mask);
        float overlayAlpha = (mask * 0.62 + halo * 0.18) * uOverlayOpacity;
        vec3 inverted = vec3(1.0) - background;
        vec3 partialInvert = mix(background, inverted, 0.42);
        vec3 effectColor = mix(partialInvert, background, 0.12);
        FragColor = vec4(effectColor, overlayAlpha);
        return;
    }

    FragColor = vec4(color.rgb, color.a * uOverlayOpacity);
}
