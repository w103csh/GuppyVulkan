
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform MaterialDefault {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    uint shininess;     // Specular shininess factor
    uint texFlags;      // Texture flags
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
} material[1];
// IN
layout(location = 0) in vec3 fragPos;       // not used
layout(location = 1) in vec3 fragNormal;    // not used
layout(location = 2) in vec4 fragColor;
// OUT
layout(location = 0) out vec4 outColor;

void main() {
    uint i = material[0].flags;
    outColor = fragColor;
}