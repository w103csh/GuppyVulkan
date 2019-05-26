
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define DSMI_UNI_PBR 0

// MATERIAL FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;
const uint METAL                = 0x00001000u;

layout(set=DSMI_UNI_PBR, binding=1) uniform MaterialPBR {
    vec3 color;         // Diffuse color for dielectrics, f0 for metallic
    float opacity;      // Overall opacity
    // 16
    uint flags;         // Flags (general/material)
    uint texFlags;      // Flags (texture)
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
    // 16
    float rough;        // Roughness
    // rem 12
} material;

vec3 getMaterialColor() { return material.color; }
uint getMaterialFlags() { return material.flags; }
uint getMaterialTexFlags() { return material.texFlags; }
float getMaterialOpacity() { return material.opacity; }
float getMaterialXRepeat() { return material.xRepeat; }
float getMaterialYRepeat() { return material.yRepeat; }
float getMaterialRough() { return material.rough; }

// FLAG CHECKS
bool isPerMaterialColor() { return (material.flags & PER_MATERIAL_COLOR) > 0; }
bool isPerVertexColor() { return (material.flags & PER_VERTEX_COLOR) > 0; }
bool isPerTextureColor() { return (material.flags & PER_TEXTURE_COLOR) > 0; }
bool isMetal() { return (material.flags & METAL) > 0; }
