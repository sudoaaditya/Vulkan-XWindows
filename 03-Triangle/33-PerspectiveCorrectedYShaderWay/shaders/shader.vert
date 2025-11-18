#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPosition;

layout(binding = 0) uniform mvpMatrix {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} uMVP;

void main (void) {

    // code
    gl_Position = uMVP.projectionMatrix * uMVP.viewMatrix * uMVP.modelMatrix * vPosition;
    gl_Position.y = -gl_Position.y; // flip y coordinate to correct perspective
}
