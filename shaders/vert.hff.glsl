/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_HFF_DEF 0

// BINDINGS
layout(set=_DS_HFF_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_HFF_DEF, binding=2) uniform Simulation {
    float c2;        // wave speed
    float h;         // distance between heights
    float h2;        // h squared
    float dt;        // time delta
    float maxSlope;  // clamped sloped to prevent numerical explosion
    int read, write;
    int mMinus1, nMinus1;
} sim;
layout(set=_DS_HFF_DEF, binding=3, r32f) uniform readonly image3D imgHeightField;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in ivec2 inImageOffset;
layout(location=2) in vec4 inNormal;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;        // (world space)
layout(location=1) out vec3 outNormal;          // (world space)
layout(location=2) out vec4 outColor;

void main() {
    // Position
    outPosition = inPosition;
    outPosition.y += imageLoad(imgHeightField, ivec3(inImageOffset, sim.write)).r;
    outPosition = (inModel * vec4(outPosition, 1.0)).xyz;
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // Normal
    outNormal = normalize(mat3(inModel) * inNormal.xyz); // normal matrix ??
}