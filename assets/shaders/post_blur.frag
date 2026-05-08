#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uSourceTexture;
uniform int uHorizontal;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(uSourceTexture, 0));
    vec2 direction = uHorizontal == 1 ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(uSourceTexture, vTexCoord).rgb * weights[0];

    for (int index = 1; index < 5; ++index)
    {
        result += texture(uSourceTexture, vTexCoord + direction * float(index)).rgb * weights[index];
        result += texture(uSourceTexture, vTexCoord - direction * float(index)).rgb * weights[index];
    }

    FragColor = vec4(result, 1.0);
}
