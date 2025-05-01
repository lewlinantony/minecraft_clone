#version 330 core

out vec4 FragColor;

in vec2 TextCoord1;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TextCoord1);
    FragColor = texColor;
}