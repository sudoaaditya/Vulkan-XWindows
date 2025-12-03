#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 out_phong_ads_light;

layout(location = 0) out vec4 FragColor;

void main(void) {
    FragColor = vec4(out_phong_ads_light, 1.0);
}
