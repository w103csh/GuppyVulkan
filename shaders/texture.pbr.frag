/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// DECLARATIONS
vec3 pbrShade();
vec3 setTextureDefaults();
vec3 texCoordShade();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 inPosition;  // (camera space)
layout(location=1) in vec3 inNormal;    // (texture space)
layout(location=2) in vec2 inTexCoord;  // (texture space)
layout(location=3) in mat3 inTBN;
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
float opacity;
bool TEX_COORD_SHADE;

/*  TODO: these material getters show that this pattern isn't
    great. Using link shaders like this should just make adding
    new shaders easier. Anything final should be hardcoded
    into one shader file or string in the C++ shader object. */ 
vec3 getMaterialSpecular() { return vec3(1.0); }

vec3 transform(vec3 v) { return inTBN * v; }

void main() {
    setTextureDefaults();

    if (TEX_COORD_SHADE) {
        outColor = gammaCorrect(texCoordShade(), 1.0);
    } else {
        // return;
        outColor = gammaCorrect(pbrShade(), opacity);
    }
}