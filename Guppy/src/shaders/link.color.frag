
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS 
vec3 getMaterialAmbient();
vec3 getMaterialColor();
vec3 getMaterialSpecular();
uint getMaterialFlags();
float getMaterialOpacity();

// MATERIAL FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;

// IN
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;

// GLOBAL
vec3    Ka, // ambient coefficient
        Kd, // diffuse coefficient
        Ks, // specular coefficient
        n;  // normal
float opacity;

void setColorDefaults() {
    n = fragNormal;
    uint flags = getMaterialFlags();
    
    if ((flags & PER_VERTEX_COLOR) > 0) {
        Ka = fragColor.xyz;
        Kd = fragColor.xyz;
        Ks = vec3(0.8, 0.8, 0.8);
        opacity = fragColor.w;
    } else if ((flags & PER_MATERIAL_COLOR) > 0) {
        Ka = getMaterialAmbient();
        Kd = getMaterialColor();
        Ks = getMaterialSpecular();
        opacity = getMaterialOpacity();
    } else {
        // Set bad flags to red so it can be easily identified
        Ka = Kd = Ks = vec3(1.0, 0.0, 0.0);
        opacity = 1.0;
    }
}