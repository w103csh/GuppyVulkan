/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_OCEAN 0
#define _LS_X 1
#define _LS_Y 1

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform SimulationDraw {
    vec4 data0;   // [0] horizontal displacement scale factor
                  // [1] time
                  // [2] grid scale (Lx)
                  // [3] grid scale (Lz)
    uvec2 data1;  // [0] log2 of discrete dimension N
                  // [1] log2 of discrete dimension M
} sim;
layout(set=_DS_OCEAN, binding=2, rgba32f) uniform image2DArray imgDisp;
layout(set=_DS_OCEAN, binding=6, rgba32f) uniform writeonly image2DArray imgVertInput;
// IN
layout(local_size_x=_LS_X, local_size_y=_LS_Y) in;

// Dispersion relation image layers
const int DISP_LAYER_HEIGHT          = 0;
const int DISP_LAYER_SLOPE           = 1;
const int DISP_LAYER_DIFFERENTIAL    = 2;
// Vertex shader input image layers
const int INPUT_LAYER_POSITION       = 0;
const int INPUT_LAYER_NORMAL         = 1;

#define DEBUG 0

void main() {
#if DEBUG
    const ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    imageStore(imgVertInput, ivec3(pix, INPUT_LAYER_POSITION), vec4(pix.x, 0, pix.y, 1));
#else
    const ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    const bool flipSign = ((pix.x + pix.y) & 1) > 0;

    // Differential
    vec4 dxdz = imageLoad(imgDisp, ivec3(pix, DISP_LAYER_DIFFERENTIAL));
    dxdz.x = flipSign ? -dxdz.x : dxdz.x;
    dxdz.z = flipSign ? -dxdz.z : dxdz.z;

    // Position (displacement/height)
    vec3 position = vec3(
        (dxdz.x * sim.data0[0]),  // x horizontal displacement (choppiness)
        (dxdz.z * sim.data0[0]),  // z horizontal displacement (choppiness)
        imageLoad(imgDisp, ivec3(pix, DISP_LAYER_HEIGHT)).x  // height
    );
    position.z = flipSign ? -position.z : position.z;

    // Normal
    vec3 normal = imageLoad(imgDisp, ivec3(pix, DISP_LAYER_SLOPE)).xyz;
    normal = flipSign ? -normal : normal;
    normal = normalize(vec3(-normal.x, 1.0, -normal.z));

    imageStore(imgVertInput, ivec3(pix, INPUT_LAYER_POSITION), vec4(position, 1));
    imageStore(imgVertInput, ivec3(pix, INPUT_LAYER_NORMAL),   vec4(normal, 0));
#endif
}