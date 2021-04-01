/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _USE_DISP_SAMP _DISPATCH_ON_COMPUTE_QUEUE
#define _DS_OCEAN 0

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_OCEAN, binding=2) uniform SimulationDraw {
    float lambda;   // horizontal displacement scale factor
} sim;
#if _USE_DISP_SAMP
layout(set=_DS_OCEAN, binding=3) uniform sampler2DArray sampDisp;
#else
layout(set=_DS_OCEAN, binding=3, rgba32f) uniform readonly image2DArray imgDisp;
#endif

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in ivec2 inImageOffset;
layout(location=2) in vec4 inNormal;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition; // (world space)
layout(location=1) out vec3 outNormal;   // (world space)
layout(location=2) out vec4 outColor;

const int LAYER_HEIGHT          = 0;
const int LAYER_SLOPE           = 1;
const int LAYER_DIFFERENTIAL    = 2;

void main() {
    const bool flipSign = ((inImageOffset.x + inImageOffset.y) & 1) > 0;

    // Differential
#if _USE_DISP_SAMP
    vec4 dxdz = texelFetch(sampDisp, ivec3(inImageOffset, LAYER_DIFFERENTIAL), 0);
#else
    vec4 dxdz = imageLoad(imgDisp, ivec3(inImageOffset, LAYER_DIFFERENTIAL));
#endif
    dxdz.x = flipSign ? -dxdz.x : dxdz.x;
    dxdz.z = flipSign ? -dxdz.z : dxdz.z;

    // Position
    outPosition = vec3(
        inPosition.x + dxdz.x * sim.lambda,                            // x horizontal displacement (choppiness)
#if _USE_DISP_SAMP
        texelFetch(sampDisp, ivec3(inImageOffset, LAYER_HEIGHT), 0).r, // height
#else
        imageLoad(imgDisp, ivec3(inImageOffset, LAYER_HEIGHT)).r,      // height
#endif
        inPosition.z + dxdz.z * sim.lambda                             // z horizontal displacement (choppiness)
    );
    outPosition.y = flipSign ? -outPosition.y : outPosition.y;
    outPosition = (inModel * vec4(outPosition, 1.0)).xyz;
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);

    // Normal
#if _USE_DISP_SAMP
    outNormal = texelFetch(sampDisp, ivec3(inImageOffset, LAYER_SLOPE), 0).rgb;
#else
    outNormal = imageLoad(imgDisp, ivec3(inImageOffset, LAYER_SLOPE)).rgb;
#endif
    outNormal = flipSign ? -outNormal : outNormal;
    outNormal = normalize(vec3(-outNormal.x, 1.0, -outNormal.z));
    outNormal = normalize(mat3(inModel) * outNormal); // normal matrix ??
}