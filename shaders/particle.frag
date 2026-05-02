#version 410 core

out vec4 FragColor;

// RGBA: rgb = particle color, a = fade factor (1 = fresh, 0 = dead)
uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
