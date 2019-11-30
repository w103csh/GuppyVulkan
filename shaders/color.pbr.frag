/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

// DECLARATIONS
void setColorDefaults();
vec3 pbrShade();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 fragPosition;
layout(location=1) in vec3 fragNormal;
layout(location=2) in vec4 fragColor;
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
vec3    Kd, // diffuse coefficient
        n;  // normal
float opacity;

/*  TODO: these material getters show that this pattern isn't
    great. Using link shaders like this should just make adding
    new shaders easier. Anything final should be hardcoded
    into one shader file or string in the C++ shader object. */ 
vec3 getMaterialAmbient() { return vec3(1.0); }
vec3 getMaterialSpecular() { return vec3(1.0); }
vec3 transform(vec3 v) { return v; }

void main() {
    setColorDefaults();
    outColor = gammaCorrect(pbrShade(), opacity);
}