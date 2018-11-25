
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 getColor(float shininess);

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
    float shininess;     // Specular shininess factor
};

// IN
layout(location = 0) in vec3 CS_position;
layout(location = 1) in vec3 TS_normal;
layout(location = 2) in vec2 TS_texCoord;
layout(location = 3) in mat3 TBN;
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

vec3 getDirToPos(vec3 position) { return normalize(TBN * (position - CS_position)); }
vec3 getDir(vec3 direction) { return normalize(TBN * direction); }

vec3 n, Ka, Kd, Ks;

void main() {
    /*  Sampler offset if based on the Texture::FLAGS enum in C++ and
        the TEX_ constants in GLSL here. The order  in which samplerOffset
        is incremented is important and should match the C++ enum. */
    int samplerOffset = 0;
    float opacity = pushConstantsBlock.material.opacity;
    Ka = pushConstantsBlock.material.Ka;
    Kd = pushConstantsBlock.material.Kd;
    Ks = pushConstantsBlock.material.Ks;

    // Diffuse color
    if ((dynamicUbo.texFlags & TEX_DIFFUSE) > 0) {
        vec4 texDiff = texture(texSampler, vec3(TS_texCoord, samplerOffset++));
        Kd = vec3(texDiff);
        opacity = texDiff[3];
        // Use diffuse color for ambient because we haven't created
        // an ambient texture layer yet...
        Ka = Kd;
    } else if ((pushConstantsBlock.material.flags & PER_MATERIAL_COLOR) > 0) {
        // Seems superfluous...
    } else {
        // This shouldn't happen currently...
    }

    // Normal
    if ((dynamicUbo.texFlags & TEX_NORMAL) > 0) {
        vec3 texNorm = texture(texSampler, vec3(TS_texCoord, samplerOffset++)).xyz;
        n = 2.0 * texNorm - 1.0;
    } else {
	    n = TS_normal;
    }

    // Specular color
    Ks = pushConstantsBlock.material.Ks;
    if ((dynamicUbo.texFlags & TEX_SPECULAR) > 0) {
        vec4 texSpec = texture(texSampler, vec3(TS_texCoord, samplerOffset++));
        Ks = vec3(texSpec);
    }

    outColor = vec4(
        getColor(pushConstantsBlock.material.shininess),
        pushConstantsBlock.material.opacity
    );
}