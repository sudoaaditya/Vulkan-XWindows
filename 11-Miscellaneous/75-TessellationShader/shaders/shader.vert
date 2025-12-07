#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vPosition;

void main(void) {

    // code
    gl_Position = vec4(vPosition, 0.0, 1.0);
}
