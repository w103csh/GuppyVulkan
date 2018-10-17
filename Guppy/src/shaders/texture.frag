
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DIRECTION LIGHT FLAGS
const uint DL_HIDE          = 0x00000001u;
const uint DL_SHOW          = 0x00000002u;
const uint DL_BLINN_PHONG   = 0x00000010u;
const uint DL_BLINN         = 0x00000020u;

// APPLICATION CONSTANTS
// TODO: get these from a ubo or something...
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

struct DirectionalLight {
    vec3 pos;   float power;
    vec3 color; float shininess;
    vec3 diff;  uint flags;
    vec3 spec;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    DirectionalLight dirLight1;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);

        // check if light should be shown
    if ((ubo.dirLight1.flags & DL_SHOW) > 0) {
        vec3 lightDir = ubo.dirLight1.pos - fragPos;
        float distance = length(lightDir);
        distance = distance * distance;
        lightDir = normalize(lightDir);

        float lambertian = max(dot(lightDir, fragNormal), 0.0);
        float specular = 0.0;

        if(lambertian > 0.0) {
            vec3 viewDir = normalize(-fragPos);

            // this is blinn phong
            vec3 halfDir = normalize(lightDir + viewDir);
            float specAngle = max(dot(halfDir, fragNormal), 0.0);
            specular = pow(specAngle, ubo.dirLight1.shininess);
       
            // this is phong (for comparison)
            if((ubo.dirLight1.flags & DL_BLINN) > 0) {
                vec3 reflectDir = reflect(-lightDir, fragNormal);
                specAngle = max(dot(reflectDir, viewDir), 0.0);
                // note that the exponent is different here
                specular = pow(specAngle, ubo.dirLight1.shininess/4.0);
            }
        }
        vec3 colorLinear = vec3(outColor) +
                            ubo.dirLight1.diff * lambertian * ubo.dirLight1.color * ubo.dirLight1.power / distance +
                            ubo.dirLight1.spec * specular * ubo.dirLight1.color * ubo.dirLight1.power / distance;
        // apply gamma correction (assume ambientColor, diffuseColor and specColor
        // have been linearized, i.e. have no gamma correction in them)
        vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0/screenGamma));

        // use the gamma corrected color in the fragment
        outColor = vec4(colorGammaCorrected, 1.0);
    }
}