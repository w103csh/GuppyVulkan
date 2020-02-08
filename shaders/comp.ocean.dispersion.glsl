/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_OCEAN 0
#define _LS_X 1
#define _LS_Y 1

#define complexMul(a, b) vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x)

// SPECIALIZATION
layout (constant_id = 0) const float OMEGA_0    = 0.03141592653; // dispersion repeat time factor (2 * PI / T)
layout (constant_id = 1) const int N            = 256;
layout (constant_id = 2) const int M            = 256;
// BINDINGS
layout(set=_DS_OCEAN, binding=2) uniform Simulation {
    uvec2 nmLog2;   // log2 of discrete dimensions
    float lambda;   // horizontal displacement scale factor
    float t;        // time
} sim;
layout(set=_DS_OCEAN, binding=3, rgba32f) uniform image2DArray imgOcean;
layout(set=_DS_OCEAN, binding=4) uniform isamplerBuffer bitRevOffsetsN;
layout(set=_DS_OCEAN, binding=5) uniform isamplerBuffer bitRevOffsetsM;
// IN
layout(local_size_x=_LS_X, local_size_y=_LS_Y) in;

const int LAYER_WAVE            = 0;
const int LAYER_FOURIER         = 1;
const int LAYER_HEIGHT          = 2;
const int LAYER_SLOPE           = 3;
const int LAYER_DIFFERENTIAL    = 4;
const float GRAVITY = 9.81;
const float EPSILON = 1e-6;

void main() {
    const ivec2 pixRead = ivec2(gl_GlobalInvocationID.xy);

    // Do the bit reversal for the FFT here.
    const ivec2 pixWrite = ivec2(
        texelFetch(bitRevOffsetsN, int(gl_GlobalInvocationID.x)).r,
        texelFetch(bitRevOffsetsM, int(gl_GlobalInvocationID.y)).r
    );

    // Wave vector magnitude (xy: normalized wave vector, z: wave speed (m/s), w: sqrt(gravity * k magnitude))
    const vec4 kData = imageLoad(imgOcean, ivec3(pixRead, LAYER_WAVE));
    // fourier domain data (xy: hTilde0, zw: hTildeConj)
    const vec4 fourierData = imageLoad(imgOcean, ivec3(pixRead, LAYER_FOURIER));

    // Dispersion relation
    const float omega_kt = floor(kData.w / OMEGA_0) // take the integer part of [[a]]
        * OMEGA_0 * sim.t;

    const float cos_omega_kt = cos(omega_kt);
    const float sin_omega_kt = sin(omega_kt);

    const vec2 hTilde = complexMul(fourierData.xy, vec2(cos_omega_kt,  sin_omega_kt)) +
                        complexMul(fourierData.zw, vec2(cos_omega_kt, -sin_omega_kt));

    // Height
    imageStore(imgOcean, ivec3(pixWrite, LAYER_HEIGHT), vec4(hTilde, 0, 0));

    // Slope
    imageStore(imgOcean, ivec3(pixWrite, LAYER_SLOPE), vec4(
        complexMul(hTilde, vec2(0.0, kData.x)),
        complexMul(hTilde, vec2(0.0, kData.y))
    ));

    // Differentials
    if (kData.z < EPSILON) {
        imageStore(imgOcean, ivec3(pixWrite, LAYER_DIFFERENTIAL), vec4(0,0,0,0));
    } else {
        imageStore(imgOcean, ivec3(pixWrite, LAYER_DIFFERENTIAL), vec4(
            complexMul(hTilde, vec2(0.0, -kData.x / kData.z)),
            complexMul(hTilde, vec2(0.0, -kData.y / kData.z))
        ));
    }
}