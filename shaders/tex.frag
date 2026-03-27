#version 410 core

in vec2 fragTexCoord;

uniform sampler2D tex0; // bound to texture unit 0 from C++

out vec4 FragColor;

void main()
{
    FragColor = texture(tex0, fragTexCoord);
}
