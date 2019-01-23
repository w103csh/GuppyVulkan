
#version 450
#extension GL_ARB_separate_shader_objects : enable

// MATERIAL FLAGS
const uint METAL   = 0x00001000u;

struct PBRMaterial {
    vec3 color;         // Diffuse color for dielectrics, f0 for metallic
    uint flags;         // Metallic (true) or dielectric (false)
    float rough;        // Roughness
    // rem 16
};

// IN
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
    PBRMaterial material;
} pushConstantsBlock;
// OUT
layout(location = 0) out vec4 outColor;

void main() {
    if ((pushConstantsBlock.material.flags & METAL) > 0) {
        outColor = vec4(1.0, 0.7, 0.7, 1.0);
        return;
    }
    outColor = vec4(pushConstantsBlock.material.color.xyz, 1.0);
}