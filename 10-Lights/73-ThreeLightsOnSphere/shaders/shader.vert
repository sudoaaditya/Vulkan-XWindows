#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;

layout(binding = 0) uniform myUniformData {
    // Matrices
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    // Light A
    vec4 lightAmbientA;
    vec4 lightDiffuseA;
    vec4 lightSpecularA;
    vec4 lightPositionA;
    // Light B
    vec4 lightAmbientB;
    vec4 lightDiffuseB;
    vec4 lightSpecularB;
    vec4 lightPositionB;
    // Light C
    vec4 lightAmbientC;
    vec4 lightDiffuseC;
    vec4 lightSpecularC;
    vec4 lightPositionC;
    // Material
    vec4 materialAmbient;
    vec4 materialDiffuse;
    vec4 materialSpecular;
    float materialShininess;
    // Key Pressed Uniform
    uint lKeyPressed;
} uMyUniforms;

layout(location = 0) out vec3 out_tNormal;
layout(location = 1) out vec3 out_viewerVector;
layout(location = 2) out vec3 out_lightDirectionA;
layout(location = 3) out vec3 out_lightDirectionB;
layout(location = 4) out vec3 out_lightDirectionC;

void main (void) {
    // code
    if(uMyUniforms.lKeyPressed == 1) {
        vec4 eyeCoords = uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vPosition;
        out_tNormal = mat3(uMyUniforms.viewMatrix * uMyUniforms.modelMatrix) * vNormal;

        out_lightDirectionA = vec3(uMyUniforms.lightPositionA - eyeCoords);
        out_lightDirectionB = vec3(uMyUniforms.lightPositionB - eyeCoords);
        out_lightDirectionC = vec3(uMyUniforms.lightPositionC - eyeCoords);

        out_viewerVector = vec3(-eyeCoords.xyz);
    }
    gl_Position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vPosition;
}
