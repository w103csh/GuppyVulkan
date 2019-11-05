
#version 450

#define _DS_UNI_DEF 0

// FLAGS
const uint PER_MATERIAL_COLOR       = 0x00000001u;
const uint PER_VERTEX_COLOR         = 0x00000002u;
const uint PER_TEXTURE_COLOR        = 0x00000004u;
const uint MODE_TOON_SHADE          = 0x00000040u;
const uint SKYBOX                   = 0x00000200u;
const uint REFLECT                  = 0x00000400u;
const uint REFRACT                  = 0x00000800u;
const uint IS_MESH                  = 0x00010000u;

// TEXTURE
//  TYPE MASK
const uint TEX_HEIGHT           = 0x000F0000u;

layout(set=_DS_UNI_DEF, binding=1) uniform MaterialDefault {
    vec3 color;             // Diffuse color for dielectrics, f0 for metallic
    float opacity;          // Overall opacity
    // 16
    uint flags;             // Flags (general/material)
    uint texFlags;          // Flags (texture)
    float xRepeat;          // Texture xRepeat
    float yRepeat;          // Texture yRepeat
    // 16
    vec3 Ka;                // Ambient reflectivity
    float shininess;        // Specular shininess factor
    // 16
    vec3 Ks;                // Specular reflectivity
    float eta;              // Index of refraction
    // 16
    float reflectionFactor; // Percentage of reflected light
    float _padding0;
    float _padding1;
    float _padding2;

    // OBJ3D
    mat4 model;
} material;

vec3  getMaterialAmbient()          { return material.Ka; }
vec3  getMaterialColor()            { return material.color; }
vec3  getMaterialSpecular()         { return material.Ks; }
uint  getMaterialFlags()            { return material.flags; }
uint  getMaterialTexFlags()         { return material.texFlags; }
float getMaterialOpacity()          { return material.opacity; }
float getMaterialXRepeat()          { return material.xRepeat; }
float getMaterialYRepeat()          { return material.yRepeat; }
float getMaterialShininess()        { return material.shininess; }
float getMaterialEta()              { return material.eta; }
float getMaterialReflectionFactor() { return material.reflectionFactor; }
mat4  getModel()                    { return material.model; }

// FLAG CHECKS
// MATERIAL 
bool isPerMaterialColor()           { return (material.flags & PER_MATERIAL_COLOR) > 0; }
bool isPerVertexColor()             { return (material.flags & PER_VERTEX_COLOR) > 0; }
bool isPerTextureColor()            { return (material.flags & PER_TEXTURE_COLOR) > 0; }
bool isModeToonShade()              { return (material.flags & MODE_TOON_SHADE) > 0; }
bool isSkybox()                     { return (material.flags & SKYBOX) > 0; }
bool isReflect()                    { return (material.flags & REFLECT) > 0; }
bool isRefract()                    { return (material.flags & REFRACT) > 0; }
bool isMesh()                       { return (material.flags & IS_MESH) > 0; }
// TEXTURE
bool isTextureHeight()              { return (material.texFlags & TEX_HEIGHT) > 0; }
