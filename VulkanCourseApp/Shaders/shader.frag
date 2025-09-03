#version 450

// Interpolated color from vertex
layout(location = 0) in vec3 fragmentColor;

layout(location = 0) out vec4 outputColor;

void main()
{
    outputColor = vec4(fragmentColor, 1.0);
}