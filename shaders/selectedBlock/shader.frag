#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in float FaceID;
in vec3 Pos;

uniform sampler2D text;
uniform int blockType;

void main()
{
    float atlasSize = 1024.0;
    float texSize = 128.0f;
    float texPerRow = atlasSize/texSize;

    vec2 atlasPos;
    if (blockType == 1.0) { // Grass
        if (FaceID == 2.0) { 
            atlasPos = vec2(1.0, 0.0); // Top 
        } else if (FaceID == 3.0) {
            atlasPos = vec2(2.0, 0.0); // Bottom 
        } else {
            atlasPos = vec2(0.0, 0.0); // Sides (0.0, 1.0, 4.0, 5.0) 
        }     
    }
    else if (blockType == 2.0){ //Dirt
        atlasPos = vec2(2.0f, 0.0f);
    }
    else if (blockType == 3.0){ //Stone
        atlasPos = vec2(7.0f, 0.0f);
    }    

    // we are working with normalised values here    
    vec2 uvMin = atlasPos / texPerRow;
    vec2 uvMax = (atlasPos + vec2(1.0, 1.0)) / texPerRow;
    vec2 uv = mix(uvMin, uvMax, TexCoord);    

    vec4 texColor = texture(text, uv);

    float borderWidth = 0.003; // Controls how wide the border is (0.01-0.05 works well)
    float borderDarkness = 0.3f;    

    // Calculate distance from texture edge
    float distFromEdgeX = min(TexCoord.x, 1.0 - TexCoord.x); //Calculate distance from border
    float distFromEdgeY = min(TexCoord.y, 1.0 - TexCoord.y);
    float distFromEdge = min(distFromEdgeX, distFromEdgeY);
    
    // Create a darkening factor that increases near edges
    float borderFactor = 1.0;
    if (distFromEdge < borderWidth) { // if inside the set border width
        // Smoothly transition from border darkness to normal color
        borderFactor = mix(borderDarkness, 1.0, distFromEdge/borderWidth); // linearly interpolate it with distFromEdge/borderWidth as the scale that determines how darked it is
    }        

    // Apply the border darkening
    texColor.rgb *= borderFactor;

    float brightness = 0.8f;

    //Apply the brightness
    texColor.rgb *= brightness;

    FragColor = texColor;
}

