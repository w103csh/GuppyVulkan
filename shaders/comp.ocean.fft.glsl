/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_OCEAN 0
#define _LS_X 1

#define complexMul(a, b) vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    int rowColOffset;
} pc;
// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform SimulationDispatch {
    uvec2 nmLog2;   // log2 of discrete dimensions
    float t;        // time
} sim;
layout(set=_DS_OCEAN, binding=2, rgba32f) uniform image2DArray imgDisp;
layout(set=_DS_OCEAN, binding=5) uniform samplerBuffer sampTwiddle;
// IN
layout(local_size_x=_LS_X) in;

const float PI = 3.14159265358979323846;
const int LAYER_HEIGHT          = 0;
const int LAYER_SLOPE           = 1;
const int LAYER_DIFFERENTIAL    = 2;

void transform2(const ivec2 pixA, const ivec2 pixB, const vec2 w, const in int layer) {
    vec2 t0 = complexMul(w, imageLoad(imgDisp, ivec3(pixB, layer)).rg);
    vec2 t1 = imageLoad(imgDisp, ivec3(pixA, layer)).rg;
    imageStore(imgDisp, ivec3(pixB, layer), vec4(t1 - t0, 0, 0));
    imageStore(imgDisp, ivec3(pixA, layer), vec4(t1 + t0, 0, 0));
}

void transform4(const ivec2 pixA, const ivec2 pixB, const vec2 w, const in int layer) {
    vec4 t0 = imageLoad(imgDisp, ivec3(pixB, layer));
    t0 = vec4(complexMul(w, t0.xy), complexMul(w, t0.zw));
    vec4 t1 = imageLoad(imgDisp, ivec3(pixA, layer));
    imageStore(imgDisp, ivec3(pixB, layer), t1 - t0);
    imageStore(imgDisp, ivec3(pixA, layer), t1 + t0);
}

void main() {
    // Determine the pixel location based on the push constant offset.
    ivec2 pixA, pixB;
    pixA[pc.rowColOffset] = pixB[pc.rowColOffset] = int(gl_GlobalInvocationID.x);

    int i, m, m2, j, k,
        offset = pc.rowColOffset ^ 1,
        n = imageSize(imgDisp)[offset];

    vec2 twiddle;

    for (int s = 1; s <= sim.nmLog2[offset]; ++s) {
        m = 1 << s;
        m2 = m >> 1;
        for (j = 0; j < m2; ++j) {
            twiddle = texelFetch(sampTwiddle, m - m2 + j - 1).rg;
            for (k = j; k < n; k += m) {
                pixA[offset] = k;
                pixB[offset] = k + m2;
                transform2(pixA, pixB, twiddle, LAYER_HEIGHT);
                transform4(pixA, pixB, twiddle, LAYER_SLOPE);
                transform4(pixA, pixB, twiddle, LAYER_DIFFERENTIAL);
            }
        }
    }
}