#version 450

layout(location = 0) in vec3 fragmentColor;
layout(location = 0) out vec4 outputColor;                  // Final output color

void main()
{
    outputColor = vec4(fragmentColor, 1.0);
}