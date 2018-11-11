
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 getColor(uint shininess);

// MATERIAL FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;

// DYNAMIC UNIFORM BUFFER FLAGS
// TEXTURE
const uint TEX_DIFFUSE      = 0x00000001u;
const uint TEX_NORMAL       = 0x00000010u;
const uint TEX_SPECULAR     = 0x00000100u;

// APPLICATION CONSTANTS
// TODO: get these from a ubo or something...
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

struct Material {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    uint shininess;     // Specular shininess factor
};

struct Camera {
	mat4 mvp;
	vec3 position;
};

// IN
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
    Material material;
} pushConstantsBlock;
// TEXTURE SAMPLER
layout(binding = 1) uniform sampler2DArray texSampler;
// DYNAMIC UNIFORM BUFFER
layout(binding = 2) uniform DynamicUniformBuffer {
    uint texFlags;
} dynamicUbo;
// OUT
layout(location = 0) out vec4 outColor;

vec3 n, v, Ka, Kd, Ks;

void main() {
    float opacity;
    int samplerOffset = 0;

    // Diffuse color
    vec4 diffuseColor = vec4(0, 0, 0, 1.0);
    if ((dynamicUbo.texFlags & TEX_DIFFUSE) > 0) {
        vec4 diffColor = texture(texSampler, vec3(fragTexCoord, samplerOffset++));
        Kd = vec3(diffColor);
        opacity = diffColor[3];
    } else if ((pushConstantsBlock.material.flags & PER_MATERIAL_COLOR) > 0) {
        Kd = pushConstantsBlock.material.Kd;
        opacity = pushConstantsBlock.material.opacity;
    } else {
        // This shouldn't happen at this point...
        opacity = pushConstantsBlock.material.opacity;
    }

    // Ambient color
    Ka = Kd;

    // Specular color
    vec4 specularColor = vec4(0, 0, 0, 1.0);
    if ((dynamicUbo.texFlags & TEX_SPECULAR) > 0) {
        Ks = vec3(texture(texSampler, vec3(fragTexCoord, samplerOffset++)));
        // TODO: should we use the 4th component here???
    } else {
        Ks = pushConstantsBlock.material.Kd;
    }

    // Normal
    if ((dynamicUbo.texFlags & TEX_NORMAL) > 0) {
        n = normalize(vec3(texture(texSampler, vec3(fragTexCoord, samplerOffset++))));
    } else {
	    n = fragNormal;
    }

    outColor = vec4(
        getColor(pushConstantsBlock.material.shininess),
        pushConstantsBlock.material.opacity
    );
}