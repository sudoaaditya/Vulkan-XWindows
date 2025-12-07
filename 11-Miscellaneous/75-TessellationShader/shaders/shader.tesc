#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(vertices = 4) out;

layout(binding = 0) uniform Uniforms {
    mat4 mvpMatrix;
    vec4 numberOfLineSegments;
    vec4 numberOfLineStrips;
    vec4 lineColor;
} uniforms;

void main(void) {

    gl_TessLevelOuter[0] = uniforms.numberOfLineStrips.x;
    gl_TessLevelOuter[1] = uniforms.numberOfLineSegments.x;

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}