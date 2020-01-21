/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_OCEAN 0

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_OCEAN, binding=3, rgba32f) uniform readonly image2DArray imgOcean;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in ivec2 inImageOffset;
layout(location=2) in vec4 inNormal;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;        // (world space)
layout(location=1) out vec3 outNormal;          // (world space)
layout(location=2) out vec4 outColor;

const int LAYER_HEIGHT = 2;

void main() {
    // Position
    outPosition = inPosition;
    outPosition.y += imageLoad(imgOcean, ivec3(inImageOffset, LAYER_HEIGHT)).r;
    if (((inImageOffset.x + inImageOffset.y) & 1) > 0)
        outPosition.y = -outPosition.y;
    outPosition = (inModel * vec4(outPosition, 1.0)).xyz;
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // Normal
    // outNormal = normalize(mat3(inModel) * inNormal.xyz); // normal matrix ??
    outNormal = normalize(mat3(inModel) * vec3(0, 1, 0));
}