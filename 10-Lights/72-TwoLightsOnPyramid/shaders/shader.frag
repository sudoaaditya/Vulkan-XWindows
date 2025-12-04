#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 out_tNormal;
layout(location = 1) in vec3 out_viewerVector;
layout(location = 2) in vec3 out_lightDirectionA;
layout(location = 3) in vec3 out_lightDirectionB;

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
    // Material
    vec4 materialAmbient;
    vec4 materialDiffuse;
    vec4 materialSpecular;
    float materialShininess;
    // Key Pressed Uniform
    uint lKeyPressed;
} uMyUniforms;


layout(location = 0) out vec4 FragColor;

void main(void) {
    vec3 phong_ads_light;
    if(uMyUniforms.lKeyPressed == 1) {
        vec3 n_tNormal = normalize(out_tNormal);
        vec3 n_viewerVec = normalize(out_viewerVector);

        vec3 n_lightDirectionA = normalize(out_lightDirectionA);
        float tn_dot_ldA = max(dot(n_lightDirectionA, n_tNormal), 0.0);
        vec3 reflectionVectorA = reflect(-n_lightDirectionA, n_tNormal);
        float rv_dot_vv_pow_shine_a = pow(max(dot(reflectionVectorA, n_viewerVec), 0.0), uMyUniforms.materialShininess);

        vec3 n_lightDirectionB = normalize(out_lightDirectionB);
        float tn_dot_ldB = max(dot(n_lightDirectionB, n_tNormal), 0.0);
        vec3 reflectionVectorB = reflect(-n_lightDirectionB, n_tNormal);
        float rv_dot_vv_pow_shine_b = pow(max(dot(reflectionVectorB, n_viewerVec), 0.0), uMyUniforms.materialShininess);

        vec3 ambientA = vec3(uMyUniforms.lightAmbientA * uMyUniforms.materialAmbient);
        vec3 diffuseA = vec3(uMyUniforms.lightDiffuseA * uMyUniforms.materialDiffuse * tn_dot_ldA);
        vec3 specularA = vec3(uMyUniforms.lightSpecularA * uMyUniforms.materialSpecular * rv_dot_vv_pow_shine_a);

        vec3 ambientB = vec3(uMyUniforms.lightAmbientB * uMyUniforms.materialAmbient);
        vec3 diffuseB = vec3(uMyUniforms.lightDiffuseB * uMyUniforms.materialDiffuse * tn_dot_ldB);
        vec3 specularB = vec3(uMyUniforms.lightSpecularB * uMyUniforms.materialSpecular * rv_dot_vv_pow_shine_b);
        
        phong_ads_light = (ambientA + diffuseA + specularA) + (ambientB + diffuseB + specularB);
    }
    else {
        phong_ads_light = vec3(1.0, 1.0, 1.0);
    }
    FragColor = vec4(phong_ads_light, 1.0);
}
