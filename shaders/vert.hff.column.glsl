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

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pc;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in ivec2 inImageOffset;

// OUT
layout(location=0) out vec3 outPosition;        // (world space)
layout(location=1) out vec3 outNormal;          // (world space)
layout(location=2) out vec4 outColor;

void main() {
    outPosition = inPosition;

    const float _h = sim.h * 0.5f; 

    float _u = 0.0f;
    float u = imageLoad(imgHeightField, ivec3(inImageOffset, sim.write)).r;
    u = max(u, 0.02f);

    if (gl_VertexIndex == 0
        || gl_VertexIndex == 19
        || gl_VertexIndex == 23
        || gl_VertexIndex == 32
        || gl_VertexIndex == 34
    ) {
        // front ll, left lr, bottom ul
        outPosition.x += -_h;
        outPosition.z += _h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 1
        || gl_VertexIndex == 5
        || gl_VertexIndex == 6
        || gl_VertexIndex == 33
    ) {
        // front lr, right ll, bottom ur
        outPosition.x += _h;
        outPosition.z += _h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 2
        || gl_VertexIndex == 4
        || gl_VertexIndex == 21
        || gl_VertexIndex == 24
    ) {
        // front ul, left ur, top ll
        outPosition.x += -_h;
        outPosition.z += _h;
        outPosition.y += u;
    } else if (gl_VertexIndex == 3
        || gl_VertexIndex == 8
        || gl_VertexIndex == 10
        || gl_VertexIndex == 25
        || gl_VertexIndex == 29
    ) {
        // front ur, right ul, top lr
        outPosition.x += _h;
        outPosition.z += _h;
        outPosition.y += u;
    } else if (gl_VertexIndex == 7
        || gl_VertexIndex == 11
        || gl_VertexIndex == 12
        || gl_VertexIndex == 31
        || gl_VertexIndex == 35
    ) {
        // right lr, back ll, bottom lr
        outPosition.x += _h;
        outPosition.z += -_h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 9
        || gl_VertexIndex == 14
        || gl_VertexIndex == 16
        || gl_VertexIndex == 27
    ) {
        // right ur, back ul, top ur
        outPosition.x += _h;
        outPosition.z += -_h;
        outPosition.y += u;
    } else if (gl_VertexIndex == 13
        || gl_VertexIndex == 17
        || gl_VertexIndex == 18
        || gl_VertexIndex == 30
    ) {
        // back lr, left ll, bottom ll
        outPosition.x += -_h;
        outPosition.z += -_h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 15
        || gl_VertexIndex == 20
        || gl_VertexIndex == 22
        || gl_VertexIndex == 26
        || gl_VertexIndex == 28
    ) {
        // back ur, left ul, top ul
        outPosition.x += -_h;
        outPosition.z += -_h;
        outPosition.y += u;
    }

    if (gl_VertexIndex < 6) {
        outNormal = vec3(0.0, 0.0, 1.0); // front
    } else if (gl_VertexIndex < 12) {
        outNormal = vec3(1.0, 0.0, 0.0); // right
    } else if (gl_VertexIndex < 18) {
        outNormal = vec3(0.0, 0.0, -1.0); // back
    } else if (gl_VertexIndex < 24) {
        outNormal = vec3(-1.0, 0.0, 0.0); // left
    } else if (gl_VertexIndex < 30) {
        outNormal = vec3(0.0, 1.0, 0.0); // top
    } else {
        outNormal = vec3(0.0, -1.0, 0.0); // bottom
    }

    outPosition = (pc.model * vec4(outPosition, 1.0)).xyz;
    outNormal = mat3(pc.model) * outNormal;
    outColor = vec4(1,0,0,1);

    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
}