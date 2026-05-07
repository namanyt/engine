#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aColor;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec4 vColor;
out vec4 vShadowPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightViewProjection;

void main()
{
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));

    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize(normalMatrix * aNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;
    vShadowPosition = uLightViewProjection * worldPosition;

    gl_Position = uProjection * uView * worldPosition;
}
