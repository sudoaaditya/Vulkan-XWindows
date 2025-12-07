#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform Uniforms {
    mat4 mvpMatrix;
    vec4 numberOfLineSegments;
    vec4 numberOfLineStrips;
    vec4 lineColor;
} uniforms;

void main(void) {

    FragColor = uniforms.lineColor;
}
