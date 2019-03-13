
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define UMI_LGT_PBR_POS 0

// MATERIAL FLAGS
const uint METAL   = 0x00001000u;

// BINDINGS
layout(binding = 1) uniform MaterialPBR {
    vec3 color;         // Diffuse color for dielectrics, f0 for metallic
    uint flags;         // Metallic (true) or dielectric (false)
    float rough;        // Roughness
    // rem 16
} material;
#if UMI_LGT_PBR_POS
layout(binding = 2) uniform LightDefaultPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    vec3 La;        // Ambient light intesity
    vec3 L;         // Diffuse and specular light intensity
} lgtPos[UMI_LGT_PBR_POS];
#endif
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pushConstantsBlock;
// IN
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
// OUT
layout(location = 0) out vec4 outColor;

void main() {
    if ((material.flags & METAL) > 0) {
        outColor = vec4(0.0, 0.7, 0.7, 1.0);
        return;
    }
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
    return;
}