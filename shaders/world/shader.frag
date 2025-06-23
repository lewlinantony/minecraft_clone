#version 330 core

out vec4 FragColor;

// Attributes from Vertex Shader
in vec2 TexCoord;
in float FaceID;
in float blockType;
in vec3 FragPos; // World space position
in vec3 Normal;  // World space normal

// Texture and Material
uniform sampler2D text;

// Lighting Uniforms
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main()
{
    // 1. Texture mapping
    float atlasSize = 512.0;
    float texSize = 64.0f;
    float texPerRow = atlasSize / texSize;

    vec2 atlasPos;
    if (blockType == 1.0) { // Grass
        if (FaceID == 0.0) atlasPos = vec2(0.0, 0.0);      // Top
        else if (FaceID >= 1.0 && FaceID <= 4.0) atlasPos = vec2(1.0, 0.0); // Sides
        else atlasPos = vec2(2.0, 0.0);                    // Bottom
    } else if (blockType == 2.0) { // Dirt
        atlasPos = vec2(2.0, 0.0);
    } else if (blockType == 3.0) { // Stone
        atlasPos = vec2(3.0, 0.0);
    }

    vec2 uvMin = atlasPos / texPerRow;
    vec2 uvMax = (atlasPos + vec2(1.0, 1.0)) / texPerRow;
    vec2 uv = mix(uvMin, uvMax, TexCoord);
    vec4 texColor = texture(text, uv);

    // --- YOUR BORDER CODE ---
    float borderWidth = 0.003; 
    float borderDarkness = 0.3f;   

    float distFromEdgeX = min(TexCoord.x, 1.0 - TexCoord.x);
    float distFromEdgeY = min(TexCoord.y, 1.0 - TexCoord.y);
    float distFromEdge = min(distFromEdgeX, distFromEdgeY);
    
    float borderFactor = 1.0;
    if (distFromEdge < borderWidth) {
        borderFactor = mix(borderDarkness, 1.0, distFromEdge / borderWidth);
    }       
    
    // Apply the border darkening FIRST
    texColor.rgb *= borderFactor;

    // --- PHONG LIGHTING CALCULATION ---
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.2; 
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0); // 32 is shininess
    vec3 specular = specularStrength * spec * lightColor;
    
    // --- COMBINE RESULTS ---
    // Apply lighting to the (already border-darkened) texture color
    vec3 lighting = (ambient + diffuse + specular);
    vec3 result = lighting * texColor.rgb;

    FragColor = vec4(result, texColor.a);
}