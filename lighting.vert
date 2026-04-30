#version 460 core

// Vertex attributes
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;

// Matrices 
uniform mat4 uM_m, uV_m, uP_m;

// Outputs to the fragment shader
out VS_OUT {
    vec3 N;
    vec3 V;
    vec3 P;
    vec2 texCoord;
} vs_out;

void main(void) {
    // Create Model-View matrix
    mat4 mv_m = uV_m * uM_m;

    // Calculate view-space coordinate
    vec4 P = mv_m * vec4(aPos, 1.0f);

    // Calculate normal in view space
    vs_out.N = mat3(mv_m) * aNorm;
    // Calculate view vector (negative of the view-space position)
    vs_out.V = -P.xyz;
    // Store view-space position for per-fragment lighting computation
    vs_out.P = P.xyz;

	// Assign texture coordinates
    vs_out.texCoord = aTex;

    // Calculate the clip-space position of each vertex
    gl_Position = uP_m * P;
}
