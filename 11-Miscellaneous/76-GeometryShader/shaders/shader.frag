#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0)in vec4 trianglecolor;

layout(location = 0) out vec4 FragColor;

void main(void) {

    FragColor = trianglecolor;
}
