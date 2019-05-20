
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS 
vec3 getMaterialAmbient();
vec3 getMaterialColor();
vec3 getMaterialSpecular();
uint getMaterialFlags();
float getMaterialOpacity();
bool isPerMaterialColor();
bool isPerVertexColor();

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
    
    if (isPerVertexColor()) {
        Ka = fragColor.xyz;
        Kd = fragColor.xyz;
        Ks = vec3(0.8, 0.8, 0.8);
        opacity = fragColor.w;
    } else if (isPerMaterialColor()) {
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