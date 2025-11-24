#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform mvpMatrix {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} uMVP;

void main (void) {

    // code
    gl_Position = uMVP.projectionMatrix * uMVP.viewMatrix * uMVP.modelMatrix * vPosition;

    outColor = vColor;
}
