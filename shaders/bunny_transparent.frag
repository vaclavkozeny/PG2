#version 410 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform float iTime;

void main() {
    // Soft animated tint with fixed transparency
    vec2 uv = vTexCoord * 2.0f - 1.0f;
    float ring = 0.5f + 0.5f * sin(8.0f * length(uv) - iTime * 2.5f);

    vec3 base = vec3(
        0.4f + 0.6f * sin(iTime + uv.x * 2.0f),
        0.4f + 0.6f * sin(iTime + uv.y * 2.0f),
        0.6f + 0.4f * sin(iTime * 1.3f)
    );

    vec3 color = base * ring;
    FragColor = vec4(color, 0.35f);
}
