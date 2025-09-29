#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform ModelViewProjection {
    mat4 projection;
    mat4 view;
    mat4 model;
} modelViewProjection;

layout(location = 0) out vec3 fragmentColor;


void main()
{
    gl_Position = modelViewProjection.projection * modelViewProjection.view * modelViewProjection.model * vec4(position, 1.0);

    fragmentColor = color;
}