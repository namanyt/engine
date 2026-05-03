#version 330 core

in vec2 vLocalPosition;

out vec4 FragColor;

uniform float uTime;
uniform vec2 uResolution;

void main()
{
    vec2 uv = vLocalPosition * 0.5 + 0.5;
    float pulse = 0.5 + 0.5 * sin(uTime * 1.6);
    float ribbon = 0.5 + 0.5 * sin((uv.x + uv.y + uTime * 0.35) * 8.0);
    float glowCenter = smoothstep(0.62, 0.14, length(vLocalPosition - vec2(0.0, -0.08)));
    float aspect = uResolution.x / max(uResolution.y, 1.0);

    vec3 sunrise = vec3(0.98, 0.47, 0.16);
    vec3 horizon = vec3(0.13, 0.54, 0.94);
    vec3 greetingGlow = vec3(1.00, 0.90, 0.42);

    vec3 color = mix(sunrise, horizon, uv.y);
    color = mix(color, greetingGlow, ribbon * 0.30 + glowCenter * 0.45 * pulse);
    color += vec3(uv.x * 0.06, 0.02 * pulse, (1.0 - uv.x) * 0.10);
    color *= mix(0.92, 1.05, clamp(aspect / 1.3333, 0.0, 1.5));

    FragColor = vec4(color, 1.0);
}
