#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(isolines) in;

layout(binding = 0) uniform Uniforms {
    mat4 mvpMatrix;
    vec4 numberOfLineSegments;
    vec4 numberOfLineStrips;
    vec4 lineColor;
} uniforms;

void main(void) {

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    vec3 p = p0 * (1.0 - gl_TessCoord.x) * (1.0 - gl_TessCoord.x) * (1.0 - gl_TessCoord.x) +
                p1 * 3.0 * gl_TessCoord.x * (1.0 - gl_TessCoord.x) * (1.0 - gl_TessCoord.x) +
                p2 * 3.0 * gl_TessCoord.x * gl_TessCoord.x * (1.0 - gl_TessCoord.x) +
                p3 * gl_TessCoord.x * gl_TessCoord.x * gl_TessCoord.x;

    gl_Position = uniforms.mvpMatrix * vec4(p, 1.0);
}