
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 getColor(uint shininess);

// MATERIAL FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;

struct Material {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    uint shininess;     // Specular shininess factor
	float xRepeat;		// Texture xRepeat
	float yRepeat;		// Texture yRepeat
};

// IN
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
    Material material;
} pushConstantsBlock;
// OUT
layout(location = 0) out vec4 outColor;

vec3 n, Ka, Kd, Ks;

void main() {
    float opacity;

    // Get the colors per vertex
    if ((pushConstantsBlock.material.flags & PER_VERTEX_COLOR) > 0) {
        Ka = vec3(fragColor);
        Kd = vec3(fragColor);
        Ks = vec3(0.8, 0.8, 0.8);
        opacity = fragColor[3];
    // Get the colors per material
    } else if ((pushConstantsBlock.material.flags & PER_MATERIAL_COLOR) > 0) {
        Ka = pushConstantsBlock.material.Ka;
        Kd = pushConstantsBlock.material.Kd;
        Ks = pushConstantsBlock.material.Ks;
        opacity = pushConstantsBlock.material.opacity;
    }

    // Normal
    n = fragNormal;
    
    outColor = vec4(
        getColor(pushConstantsBlock.material.shininess),
        pushConstantsBlock.material.opacity
    );
}