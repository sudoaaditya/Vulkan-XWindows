#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;

layout(binding = 0) uniform myUniformData {
    // Matrices Unifrom
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    // Light Uniform
    vec4 lightDiffuse;
    vec4 lightPosition;
    vec4 materialDiffuse;
    // Key Pressed Uniform
    uint lKeyPressed;
} uMyUniforms;

layout(location = 0) out vec3 out_diffiseColor;

void main (void) {

    // code
    if(uMyUniforms.lKeyPressed == 1) {
        vec4 eyeCoordinates = uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vPosition;
        mat3 normalMatrix = mat3(transpose(inverse(uMyUniforms.viewMatrix * uMyUniforms.modelMatrix)));
        vec3 transformedNormal = normalize(normalMatrix * vNormal);
        vec3 lightDirection = normalize(vec3(uMyUniforms.lightPosition) - eyeCoordinates.xyz);
        out_diffiseColor = vec3(uMyUniforms.lightDiffuse) * vec3(uMyUniforms.materialDiffuse) 
            * max(dot(lightDirection, transformedNormal), 0.0);
    } else {
        out_diffiseColor = vec3(1.0, 1.0, 1.0);
    }

    gl_Position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vPosition;
}
