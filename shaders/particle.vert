#version 410 core

// Only position is needed — particles are unlit solid-color shapes
layout(location = 0) in vec3 position;

uniform mat4 uM_m;
uniform mat4 uV_m;
uniform mat4 uP_m;

void main() {
    gl_Position = uP_m * uV_m * uM_m * vec4(position, 1.0);
}
