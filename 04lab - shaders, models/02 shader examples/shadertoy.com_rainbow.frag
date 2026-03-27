const float pi = 3.1415f;

void mainImage(out vec4 c_out, in vec2 f) {
    const float pi3   = pi/3.0f;
    const float d_pi3 = pi3*2.0f;
  
    c_out =  vec4(abs(sin(        gl_FragCoord.y*0.1f + 3.0f*iTime)),
                  abs(sin(pi3   + gl_FragCoord.y*0.1f + 3.0f*iTime)),
                  abs(sin(d_pi3 + gl_FragCoord.y*0.1f + 3.0f*iTime)),
    1.0f);
}

