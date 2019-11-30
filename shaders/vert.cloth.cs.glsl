/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DEF 0

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;    // (camera space)
layout(location=1) out vec3 outNormal;      // (camera space)
layout(location=2) out vec2 outTexCoord;
layout(location=3) out vec3 outTangent;     // (camera space)
layout(location=4) out vec3 outBinormal;    // (camera space)
layout(location=5) out flat uint outFlags;

void main() {
    vec4 pos = inModel * vec4(inPosition, 1.0);
    
    gl_Position = camera.viewProjection * pos;

    mat4 mViewModel = camera.view * inModel;
    mat3 mNormal = transpose(inverse(mat3(mViewModel)));

    outPosition = (mViewModel * vec4(inPosition, 1.0)).xyz;
    outNormal = mNormal * normalize(inNormal);
    outTangent = mNormal * normalize(vec3(0,0,1));
    outBinormal = mNormal * normalize(vec3(0,0,1));
    outTexCoord = inTexCoord;
    outFlags = 0x0u;
}