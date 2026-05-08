#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uOverlayTexture;
uniform vec2 uScreenSize;
uniform vec2 uOverlaySizePixels;
uniform vec2 uOverlayMinPixels;
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

    FragColor = vec4(color.rgb, color.a * uOverlayOpacity);
}
