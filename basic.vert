#version 410 core

// input vertex attributes
in vec3 position;     // position: MUST exist (location 0)
in vec3 normal;       // normal vector (location 1)
in vec2 texture_coords; // texture coordinates (location 2)

// Transformation matrices
uniform mat4 uM_m;  // Model matrix
uniform mat4 uV_m;  // View matrix
uniform mat4 uP_m;  // Projection matrix

out vec3 fragColor;     // output color to fragment shader
out vec3 fragNormal;    // normal for fragment shader
out vec2 fragTexCoord;  // texture coordinates for fragment shader

void main()
{
    // Apply transformation: Projection * View * Model * position
    gl_Position = uP_m * uV_m * uM_m * vec4(position, 1.0f);
    
    // Pass data to fragment shader
    fragColor = vec3(abs(sin(float(gl_InstanceID))), abs(cos(float(gl_InstanceID))), 0.5f);  // Use as color
    fragNormal = normal;
    fragTexCoord = texture_coords;
}
