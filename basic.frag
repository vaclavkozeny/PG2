#version 410 core

in vec3 fragColor;  // input from vertex stage of graphics pipeline, automatically interpolated
out vec4 FragColor; // output color of current fragment: MUST be written

void main()
{
    FragColor = vec4(fragColor, 1.0f); // copy RGB color, add Alpha=1.0 (not transparent)
}
