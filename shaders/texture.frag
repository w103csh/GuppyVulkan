/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// DECLARATIONS
bool isModeFlatShade();
vec3 blinnPhongShade();
vec3 setTextureDefaults();
vec3 texCoordShade();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 inPosition;  // (camera space)
layout(location=1) in vec3 inNormal;    // (texture space)
layout(location=2) in vec2 inTexCoord;  // (texture space)
// layout(location=2) centroid in vec2 inTexCoord;  // (texture space)
layout(location=3) in mat3 TBN;
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n;      // normal
float opacity, height;
bool TEX_COORD_SHADE;

vec3 transform(vec3 v) { return TBN * v; }

void main() {
    setTextureDefaults();
    
    if (isModeFlatShade()) {
        outColor = vec4(Kd, opacity);
        // outColor = vec4(inNormal, 1.0);
    } else if (TEX_COORD_SHADE) {
        outColor = gammaCorrect(texCoordShade(), 1.0);
    } else {
        // return;
        outColor = gammaCorrect(blinnPhongShade(), opacity);
    }
}