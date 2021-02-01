/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// DECLARATIONS 
vec3 getMaterialAmbient();
vec3 getMaterialColor();
vec3 getMaterialSpecular();
uint getMaterialFlags();
float getMaterialOpacity();
bool isPerMaterialColor();
bool isPerVertexColor();

// IN
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;

// GLOBAL
vec3    Ka, // ambient coefficient
        Kd, // diffuse coefficient
        Ks, // specular coefficient
        n;  // normal
float opacity;

void setColorDefaults() {
    n = normalize(inNormal);
    
    if (isPerVertexColor()) {
        Ka = inColor.xyz;
        Kd = inColor.xyz;
        Ks = vec3(0.8, 0.8, 0.8);
        opacity = inColor.w;
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