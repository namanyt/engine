#version 330 core

in vec2 vTexCoord;

layout (location = 0) out vec4 FragColor;

uniform sampler2D uPromptTexture;
uniform float uOpacity;

void main()
{
    vec4 color = texture(uPromptTexture, vTexCoord);
    if (color.a <= 0.01)
    {
        discard;
    }

    FragColor = vec4(color.rgb, color.a * uOpacity);
}
