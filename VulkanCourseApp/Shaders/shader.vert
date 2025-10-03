#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform ViewProjection {
    mat4 projection;
    mat4 view;
} viewProjection;

// Not In Use For Now
layout(binding = 1) uniform UniformBufferObjectModel {
    mat4 model;
} uniformBufferObjectModel;

layout(push_constant) uniform PushModel {
    mat4 model;
} pushModel;

layout(location = 0) out vec3 fragmentColor;


void main()
{
    gl_Position = viewProjection.projection * viewProjection.view * pushModel.model * vec4(position, 1.0);

    fragmentColor = color;
}