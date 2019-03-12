
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAT_DEF 1

// DECLARATIONS
vec3 getColor(float shininess);

// MATERIAL FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;

// BINDINGS
layout(binding = 1) uniform MaterialDefault {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    float shininess;    // Specular shininess factor
    uint texFlags;      // Texture flags
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
    float _padding;
} material[MAT_DEF];
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pushConstantsBlock;
// IN
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
// layout(location = 3) in flat int fragVertexIndex;
// OUT
layout(location = 0) out vec4 outColor;

vec3 getDirToPos(vec3 position) { return normalize(position - fragPos); }
vec3 getDir(vec3 direction) { return normalize(direction); }

vec3 n, Ka, Kd, Ks;

void main() {
    float opacity;

    // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    // return;
    
    // Get the colors per vertex
    if ((material[0].flags & PER_VERTEX_COLOR) > 0) {
        Ka = vec3(fragColor);
        Kd = vec3(fragColor);
        Ks = vec3(0.8, 0.8, 0.8);
        opacity = fragColor[3];
    // Get the colors per material
    } else if ((material[0].flags & PER_MATERIAL_COLOR) > 0) {
        Ka = material[0].Ka;
        Kd = material[0].Kd;
        Ks = material[0].Ks;
        opacity = material[0].opacity;
    }

    // Normal
    n = fragNormal;

    outColor = vec4(
        getColor(material[0].shininess),
        material[0].opacity
    );
}