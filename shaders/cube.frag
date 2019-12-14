/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_CBE_DEF 0

// DECLARATIONS
vec3 getMaterialColor();
float getMaterialOpacity();
float getMaterialReflectionFactor();
bool isSkybox();
bool isReflect();
bool isRefract();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

layout(set=_DS_CBE_DEF, binding=0) uniform samplerCube sampCube;
// IN
layout(location=0) in vec3 inWorldPositionReflecionDir; // (world space)
layout(location=1) in vec3 inRefractDir;                // (world space)
layout(location=2) in vec3 inPosition;                  // (camera space)
layout(location=3) in vec3 inNormal;                    // (camera space)
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
vec3    Kd;

void main() {
    vec3 sampCubeColor = texture(sampCube, inWorldPositionReflecionDir).rgb;
    if (isSkybox()) {
        outColor = gammaCorrect(sampCubeColor, getMaterialOpacity());
    } else  {
        vec3 baseColor;
        if (isReflect())
            baseColor = getMaterialColor();
        if (isRefract())
            baseColor = texture(sampCube, inRefractDir).rgb;

        /** Things to try if I ever come back to this:
        *       - Fresnel
        *       - Chromatic abberation
        *       - Refract through both sides
        *       - ...
        */

        outColor = gammaCorrect(
            mix(baseColor, sampCubeColor, getMaterialReflectionFactor()),
            getMaterialOpacity()
        );

    }
}