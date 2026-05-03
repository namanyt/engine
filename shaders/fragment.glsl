#version 330 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec4 vColor;

out vec4 FragColor;

uniform vec3 uViewPosition;

void main()
{
    vec3 normal = normalize(vWorldNormal);
    vec3 baseColor = mix(vColor.rgb, 0.35 + 0.45 * abs(normal), 0.45);
    baseColor = mix(baseColor, vec3(0.98, 0.72, 0.22), 0.20 * (vWorldPosition.y + 0.5));
    baseColor *= 0.92 + 0.08 * vec3(vTexCoord, 1.0);

    vec3 lightDirection = normalize(vec3(0.6, 1.1, 0.75));
    vec3 viewDirection = normalize(uViewPosition - vWorldPosition);
    vec3 halfVector = normalize(lightDirection + viewDirection);

    float ambient = 0.22;
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfVector), 0.0), 48.0);
    float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.5);

    vec3 color = baseColor * (ambient + diffuse * 0.90);
    color += vec3(0.95, 0.98, 1.0) * specular * 0.55;
    color += vec3(0.28, 0.55, 1.0) * rim * 0.18;

    FragColor = vec4(color, 1.0);
}
