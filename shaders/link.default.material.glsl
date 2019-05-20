
#version 450
#extension GL_ARB_separate_shader_objects : enable

// FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;
const uint MODE_TOON_SHADE      = 0x00000040u;
const uint SKYBOX               = 0x00000200u;
// TEXTURE
//  TYPE MASK
const uint TEX_HEIGHT       = 0x000F0000u;

layout(set=0, binding=1) uniform MaterialDefault {
    vec3 color;         // Diffuse color for dielectrics, f0 for metallic
    float opacity;      // Overall opacity
    // 16
    uint flags;         // Flags (general/material)
    uint texFlags;      // Flags (texture)
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
    // 16
    vec3 Ka;            // Ambient reflectivity
    float shininess;    // Specular shininess factor
    // 16
    vec3 Ks;            // Specular reflectivity
    // rem 4
} material;

vec3 getMaterialAmbient() { return material.Ka; }
vec3 getMaterialColor() { return material.color; }
vec3 getMaterialSpecular() { return material.Ks; }
uint getMaterialFlags() { return material.flags; }
uint getMaterialTexFlags() { return material.texFlags; }
float getMaterialOpacity() { return material.opacity; }
float getMaterialXRepeat() { return material.xRepeat; }
float getMaterialYRepeat() { return material.yRepeat; }
float getMaterialShininess() { return material.shininess; }

// FLAG CHECKS
bool isPerMaterialColor() { return (material.flags & PER_MATERIAL_COLOR) > 0; }
bool isPerVertexColor() { return (material.flags & PER_VERTEX_COLOR) > 0; }
bool isPerTextureColor() { return (material.flags & PER_TEXTURE_COLOR) > 0; }
bool isModeToonShade() { return (material.flags & MODE_TOON_SHADE) > 0; }
bool isSkybox() { return (material.flags & SKYBOX) > 0; }
bool isTextureHeight() { return (material.texFlags & TEX_HEIGHT) > 0; }
