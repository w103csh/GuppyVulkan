/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DEF 0
#define _DS_UNI_PRTCL_WV 0

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_UNI_PRTCL_WV, binding=0) uniform ParticleWave {
    float time;
    float freq;
    float velocity;
    float amp;
} uniWave;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec4 outColor;

const float PI = 3.1415926535897932384626433832795;
const float TAU = 2 * PI;

void main() {
    // Position
    outPosition = (inModel * vec4(inPosition, 1.0)).xyz;
    float u = (TAU / uniWave.freq) * (outPosition.x - (uniWave.velocity * uniWave.time));
    outPosition.y += uniWave.amp * sin(u);
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // Normal
    outNormal = vec3(0);
    outNormal.xy = vec2(cos(u), 1.0);
    outNormal = normalize(mat3(inModel) * inNormal); // normal matrix ??
    // Color
    outColor = inColor;
}