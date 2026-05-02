#version 410 core

// Attribute locations match glBindAttribLocation in ShaderProgram::link_shader
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coords;

uniform mat4 uM_m;
uniform mat4 uV_m;
uniform mat4 uP_m;

out vec2 fragTexCoord;

void main()
{
    gl_Position = uP_m * uV_m * uM_m * vec4(position, 1.0f);
    fragTexCoord = texture_coords;
}
