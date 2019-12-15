/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DFR_MRT 0

layout(set=_DS_UNI_DFR_MRT, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;
layout(location=5) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec2 outTexCoord;
layout(location=3) out vec3 outTangent;     // (world space)
layout(location=4) out vec3 outBinormal;    // (world space)
layout(location=5) out flat uint outFlags;

void main() {
    // Position
    outPosition = (inModel * vec4(inPosition, 1.0)).xyz;
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // Normal
    mat3 mNormal = mat3(inModel); // normal matrix ??
    outNormal = mNormal * normalize(inNormal);
    outTangent = mNormal * normalize(inTangent);
    outBinormal = mNormal * normalize(inBinormal);
    // Texture coordinate
    outTexCoord = inTexCoord;
    // Flags
    outFlags = 0x00u;
}