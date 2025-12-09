#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 out_texCoord;
layout(location = 1) in vec3 out_tNormal;
layout(location = 2) in vec3 out_viewerVector;
layout(location = 3) in vec3 out_lightDirection;

layout(binding = 0) uniform myUniformData {
    // Matrices
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    // Lights
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec4 lightSpecular;
    vec4 lightPosition;
    vec4 materialAmbient;
    vec4 materialDiffuse;
    vec4 materialSpecular;
    float materialShininess;
} uMyUniforms;

layout(binding = 1) uniform sampler2D uTextureSampler;

layout(location = 0) out vec4 FragColor;

void main(void) {
    vec3 phong_ads_light;
    vec3 n_tNormal = normalize(out_tNormal);
    vec3 n_lightDirection = normalize(out_lightDirection);
    vec3 n_viewerVec = normalize(out_viewerVector);

    float tn_dot_ld = max(dot(n_lightDirection, n_tNormal), 0.0);
    vec3 reflectionVector = reflect(-n_lightDirection, n_tNormal);
    float rv_dot_vv_pow_shine = pow(max(dot(reflectionVector, n_viewerVec), 0.0), uMyUniforms.materialShininess);

    vec3 ambient = vec3(uMyUniforms.lightAmbient * uMyUniforms.materialAmbient);
    vec3 diffuse = vec3(uMyUniforms.lightDiffuse * uMyUniforms.materialDiffuse * tn_dot_ld);
    vec3 specular = vec3(uMyUniforms.lightSpecular * uMyUniforms.materialSpecular * rv_dot_vv_pow_shine);
    
    phong_ads_light = ambient + diffuse + specular;

    vec3 textureColor = texture(uTextureSampler, out_texCoord).rgb;

    FragColor = vec4(textureColor * vec3(1.0) * phong_ads_light, 1.0);
}
