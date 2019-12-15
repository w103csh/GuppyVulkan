/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
     
#version 450

#define _DS_SMP_DEF 0

// DECLARATIONS
vec3  getMaterialColor();
uint  getMaterialFlags();
float getMaterialOpacity();
float getMaterialXRepeat();
float getMaterialYRepeat();
float getMaterialShininess();
bool  isPerTextureColor();
bool  isMesh();
bool isModeFlatShade();

layout(set=_DS_SMP_DEF, binding=0) uniform sampler2DArray sampParticle;

// IN
layout(location=0) in vec3 inPosition;          // (world space meshes, camera space quads)
layout(location=1) in vec3 inNormal;            // (world space meshes, camera space quads)
layout(location=3) in vec2 inTexCoord;
layout(location=4) in float inTransparency;
layout(location=5) in flat uint inFlags;

// OUT
layout(location=0) out vec4 outPosition;        // (world space meshes, camera space quads)
layout(location=1) out vec4 outNormal;          // (world space meshes, camera space quads)
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;
layout(location=5) out uint outFlags;

void main() {
    vec3 Kd, Ks, Ka;
    float opacity;
    bool meshFlag = isMesh();
    
    if (true && isPerTextureColor() && !meshFlag) {

        vec2 texCoord = vec2(
            (inTexCoord.x * getMaterialXRepeat()),
            (inTexCoord.y * getMaterialYRepeat())
        );

        vec4 samp = texture(sampParticle, vec3(texCoord, 0));

        Kd = samp.xyz;
        Ks = vec3(0.5);
        Ka = Kd;
        opacity = samp[3];

    } else {

        Kd = getMaterialColor();
        Ks = vec3(0.5);
        Ka = Kd;
        opacity = getMaterialOpacity();

    }

    float alpha = opacity * inTransparency;

    if (alpha < 0.15)
        discard;
    
    outPosition = vec4(inPosition, 0.0);
    outDiffuse = vec4(Kd, alpha);
    outNormal = vec4(inNormal, getMaterialShininess());
    outAmbient = vec4(Ka, alpha);
    // Remove this if I ever add the default material
    if (all(equal(Ks, vec3(0))))
        outSpecular = vec4(0.9, 0.9, 0.9, alpha);
    else
        outSpecular = vec4(Ks, alpha);
    outFlags = inFlags | (isModeFlatShade() ? 0x01u : 0x00u);
}