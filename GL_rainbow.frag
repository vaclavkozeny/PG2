#version 410 core

uniform float iTime; // input from CPU C++ App = time in seconds   

out vec4 FragColor; // output color of current fragment: MUST be written

void main() {
    const float pi    = 3.1415f;
    const float pi3   = pi/3.0f;
    const float d_pi3 = pi3*2.0f;
  
    FragColor =  vec4(abs(sin(        gl_FragCoord.y*0.1f + 3.0f*iTime)),
                      abs(sin(pi3   + gl_FragCoord.y*0.1f + 3.0f*iTime)),
                      abs(sin(d_pi3 + gl_FragCoord.y*0.1f + 3.0f*iTime)),
                      1.0f); //  add Alpha=1.0 (not transparent)
}
