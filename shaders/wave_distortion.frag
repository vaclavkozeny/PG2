#version 410 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform float iTime; // input from CPU C++ App = time in seconds

out vec4 FragColor; // output color of current fragment

void main() {
    // Use fragment coordinates + texture coordinates for wave effect
    vec2 uv = vTexCoord + gl_FragCoord.xy * 0.001f;
    
    // Create multiple wave layers
    float wave1 = sin(uv.x * 3.0f + iTime * 2.0f) * cos(uv.y * 2.0f + iTime);
    float wave2 = sin(uv.y * 2.5f - iTime * 1.5f) * cos(uv.x * 3.5f - iTime * 0.7f);
    float wave3 = sin((uv.x + uv.y) * 2.0f + iTime * 3.0f);
    
    // Use normal to modulate the effect
    float normal_influence = dot(normalize(vNormal), vec3(0.577f)); // directional bias
    
    // Combine waves into color channels
    float r = abs(sin(wave1 + iTime * 0.5f)) * 0.8f + 0.2f;
    float g = abs(sin(wave2 + iTime * 0.8f)) * 0.8f + 0.2f;
    float b = abs(sin(wave3 - iTime * 0.3f)) * 0.8f + 0.2f;
    
    // Add some pulsation
    float pulse = sin(iTime * 1.5f) * 0.3f + 0.7f;
    
    // Apply normal influence for 3D effect
    vec3 finalColor = vec3(r, g, b) * pulse;
    finalColor += normal_influence * 0.2f;
    
    FragColor = vec4(finalColor, 1.0f);
}
