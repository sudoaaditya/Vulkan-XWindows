#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;

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
    // Key Pressed Uniform
    uint lKeyPressed;
    uint isPerVertex;
} uMyUniforms;

layout(location = 0) out vec3 out_tNormal;
layout(location = 1) out vec3 out_viewerVector;
layout(location = 2) out vec3 out_lightDirection;
layout(location = 3) out vec3 out_phongADSColor;

void main (void) {
    // code
    if(uMyUniforms.lKeyPressed == 1) {
        vec4 eyeCoords = uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vPosition;
        out_tNormal = mat3(uMyUniforms.viewMatrix * uMyUniforms.modelMatrix) * vNormal;
        out_lightDirection = vec3(uMyUniforms.lightPosition - eyeCoords);
        out_viewerVector = vec3(-eyeCoords.xyz);
        if(uMyUniforms.isPerVertex == 1) {
            vec3 n_tNormal = normalize(out_tNormal);
            vec3 n_lightDirection = normalize(out_lightDirection);
            vec3 n_viewerVec = normalize(out_viewerVector);

            float tn_dot_ld = max(dot(n_lightDirection, n_tNormal), 0.0);
            vec3 reflectionVector = reflect(-n_lightDirection, n_tNormal);
            float rv_dot_vv_pow_shine = pow(max(dot(reflectionVector, n_viewerVec), 0.0), uMyUniforms.materialShininess);

            vec3 ambient = vec3(uMyUniforms.lightAmbient * uMyUniforms.materialAmbient);
            vec3 diffuse = vec3(uMyUniforms.lightDiffuse * uMyUniforms.materialDiffuse * tn_dot_ld);
            vec3 specular = vec3(uMyUniforms.lightSpecular * uMyUniforms.materialSpecular * rv_dot_vv_pow_shine);
            
            out_phongADSColor = ambient + diffuse + specular;
        } else {
            out_phongADSColor = vec3(1.0);
        }
    }
    gl_Position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vPosition;
}
