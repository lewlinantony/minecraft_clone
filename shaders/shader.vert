#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTextCoord1;

out vec3 Color;
out vec2 TextCoord1;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    Color = aColor;  // Pass color directly from vertex attributes
    TextCoord1 = aTextCoord1;
}