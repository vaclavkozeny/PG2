#version 410 core

// Attribute locations match glBindAttribLocation in ShaderProgram::link_shader
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coords;

uniform mat4 uM_m;   // Model matrix
uniform mat4 uV_m;   // View matrix
uniform mat4 uP_m;   // Projection matrix

// Outputs to fragment shader
out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    mat4 mv = uV_m * uM_m;
    vec4 P  = mv * vec4(position, 1.0);

    vFragPos  = P.xyz;
    vNormal   = mat3(mv) * normal;
    vTexCoord = texture_coords;

    gl_Position = uP_m * P;
}
