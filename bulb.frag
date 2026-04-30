#version 460 core
out vec4 FragColor;
uniform vec3 emissive_color;
uniform float emissive_intensity;
void main() {
    FragColor = vec4(emissive_color * emissive_intensity, 1.0);
}