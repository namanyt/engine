#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aColor;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec4 vColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float u_Time;
uniform float u_AnimationType;
uniform float u_TimeOffset;

mat3 rotationX(float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s, c
    );
}

mat3 rotationY(float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    return mat3(
        c, 0.0, s,
        0.0, 1.0, 0.0,
        -s, 0.0, c
    );
}

void main()
{
    float animationTime = u_Time + u_TimeOffset;

    vec3 animatedPosition = aPosition;
    vec3 animatedNormal = normalize(aNormal);

    float ripple = sin(animationTime * 2.0 + aPosition.x * 4.0 + aPosition.y * 3.0) * 0.04;
    animatedPosition += animatedNormal * ripple;

    if (u_AnimationType < 0.5)
    {
        float spin = animationTime * 1.2;
        mat3 rotation = rotationY(spin) * rotationX(spin * 0.45);
        animatedPosition = rotation * animatedPosition;
        animatedNormal = normalize(rotation * animatedNormal);
    }
    else if (u_AnimationType < 1.5)
    {
        animatedPosition.y += sin(animationTime * 2.4) * 0.30;
        animatedPosition.x += cos(animationTime * 1.3) * 0.08;
    }
    else
    {
        float pulse = 0.92 + 0.18 * sin(animationTime * 2.8);
        animatedPosition *= pulse;
    }

    vec4 worldPosition = uModel * vec4(animatedPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));

    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize(normalMatrix * animatedNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;

    gl_Position = uProjection * uView * worldPosition;
}
