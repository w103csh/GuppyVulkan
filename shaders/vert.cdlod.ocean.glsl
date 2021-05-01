/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_OCEAN 0
#define _DS_CDLOD 0

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_OCEAN, binding=2) uniform sampler2DArray sampVertInput;
layout(set=_DS_CDLOD, binding=0) uniform CdlodQuadTree {
    vec4 terrainScale;
    vec4 terrainOffset;
    vec4 heightmapTextureInfo;
    vec2 quadWorldMax;  // .xy max used to clamp triangles outside of world range
    vec2 samplerWorldToTextureScale;
    vec4 data1; // .x (dbgTexScale)
} qTree;

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    vec4 data0;  // gridDim:         .x (dimension), .y (dimension/2), .z (2/dimension)
                 //                  .w (LODLevel)
    vec4 data1;  // morph constants: .x (start), .y (1/(end-start)), .z (end/(end-start))
                 //                  .w ((aabb.minZ+aabb.maxZ)/2)
    vec4 data2;  // quadOffset:      .x (aabb.minX), .y (aabb.minY)
                 // quadScale:       .z (aabb.sizeX), .w (aabb.sizeY)
    vec4 data3;  // dbg camera:      .x (wpos.x), .y (wpos.y), .z (wpos.z)
} pc;

// IN
layout(location=0) in vec2 inPosition;
layout(location=1) in vec4 data0;      // quadOffset: .x.y
                                       // quadScale:  .z.w
layout(location=2) in vec4 data1;      // uvOffset:   .x.y
                                       // uvScale:    .z.w

// OUT
layout(location=0) out vec3 outPosition; // (world space)
layout(location=1) out vec3 outNormal;   // (world space)
layout(location=2) out vec4 outColor;

const int LAYER_POSITION  = 0;
const int LAYER_NORMAL    = 1;

#define QUAD_OFFSET_V2 data0.xy
#define QUAD_SCALE_V2  data0.zw
#define UV_OFFSET_V2   data1.xy
#define UV_SCALE_V2    data1.zw

void main() {
    // Nothing here yet.
}