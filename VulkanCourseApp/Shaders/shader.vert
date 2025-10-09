#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texture;

layout(set = 0, binding = 0) uniform ViewProjection {
    mat4 projection;
    mat4 view;
} viewProjection;

layout(push_constant) uniform PushModel {
    mat4 model;
} pushModel;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentTexture;

void main()
{
    gl_Position = viewProjection.projection * viewProjection.view * pushModel.model * vec4(position, 1.0);

    fragmentColor = color;
    fragmentTexture = texture;
}