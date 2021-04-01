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
#if _USE_DISP_SAMP
layout(set=_DS_OCEAN, binding=2) uniform sampler2DArray sampVertInput;
#else
layout(set=_DS_OCEAN, binding=2, rgba32f) uniform readonly image2DArray imgVertInput;
#endif

// IN
layout(location=0) in ivec2 inImageOffset;
layout(location=1) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition; // (world space)
layout(location=1) out vec3 outNormal;   // (world space)
layout(location=2) out vec4 outColor;

const int LAYER_POSITION  = 0;
const int LAYER_NORMAL    = 1;

void main() {
    // Position
#if _USE_DISP_SAMP
    vec4 position = texelFetch(sampVertInput, ivec3(inImageOffset, LAYER_POSITION), 0);
#else
    vec4 position = imageLoad(imgVertInput, ivec3(inImageOffset, LAYER_POSITION));
#endif
    outPosition = (inModel * position).xyz;
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);

    // Normal
#if _USE_DISP_SAMP
    outNormal = texelFetch(sampVertInput, ivec3(inImageOffset, LAYER_NORMAL), 0).xyz;
#else
    outNormal = imageLoad(imgVertInput, ivec3(inImageOffset, LAYER_NORMAL)).xyz;
#endif
    outNormal = normalize(mat3(inModel) * outNormal); // normal matrix ??
}