#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aFaceID;
layout (location = 3) in float aBlockType;
layout (location = 4) in vec3 aNormal; // New normal attribute

out vec2 TexCoord;
out float FaceID;
out float blockType;

// Outputs for lighting
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Pass-through attributes
    TexCoord = aTexCoord;
    FaceID = aFaceID;
    blockType = aBlockType;

    // Transform position and normal to world space for lighting calculations
    FragPos = vec3(model * vec4(aPos, 1.0));
    // Normals should be transformed by the normal matrix to handle non-uniform scaling correctly
    Normal = mat3(transpose(inverse(model))) * aNormal;
}