#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;

layout(binding = 1) uniform sampler2D uTextureSampler;

layout(location = 0) out vec4 FragColor;

void main(void) {

    FragColor = texture(uTextureSampler, outTexCoord);
}