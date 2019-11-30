/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

// DECLARATIONS
vec3 blinnPhongShade();
void setColorDefaults();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 fragPosition;
layout(location=1) in vec3 fragNormal;
layout(location=2) in vec4 fragColor;
// layout(location=3) in flat int fragVertexIndex;
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
vec3    Ka, // ambient coefficient
        Kd, // diffuse coefficient
        Ks, // specular coefficient
        n;  // normal
float opacity;

vec3 transform(vec3 v) { return v; }

void main() {
    setColorDefaults();
    outColor = gammaCorrect(blinnPhongShade(), opacity);
}