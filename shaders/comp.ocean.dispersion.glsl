/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_OCEAN 0

// DECLARATIONS
vec2 complexMultiply(const in vec2 a, const in vec2 b);
float complexMagnitude(const in vec2 a);

// SPECIALIZATION
layout (constant_id = 0) const float OMEGA_0 = 0.03141592653; // dispersion repeat time factor (2 * PI / T)
// BINDINGS
layout(set=_DS_OCEAN, binding=2) uniform Simulation {
    uvec2 nmLog2;   // log2 of discrete dimensions
    float lambda;   // horizontal displacement scale factor
    float t;        // time
} sim;
layout(set=_DS_OCEAN, binding=3, rgba32f) uniform image2DArray imgOcean;
// IN
layout(local_size_x=1, local_size_y=1) in;

const int LAYER_WAVE            = 0;
const int LAYER_FOURIER         = 1;
const int LAYER_HEIGHT          = 2;
const int LAYER_SLOPE           = 3;
const int LAYER_DIFFERENTIAL    = 4;
const float GRAVITY = 9.81;
const float EPSILON = 1e-6;

void main() {
    const ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    // wave vector magnitude (xy: normalized wave vector, z: wave speed (m/s), w: sqrt(gravity * k magnitude))
    const vec4 kData = imageLoad(imgOcean, ivec3(pix, LAYER_WAVE));
    // fourier domain data (xy: hTilde0, zw: hTildeConj)
    const vec4 fourierData = imageLoad(imgOcean, ivec3(pix, LAYER_FOURIER));

    // dispersion relation
    const float omega_kt = floor(kData.w / OMEGA_0) // take the integer part of [[a]]
        * OMEGA_0 * sim.t;

    const float cos_omega_kt = cos(omega_kt);
    const float sin_omega_kt = sin(omega_kt);

    const vec2 hTilde = complexMultiply(fourierData.xy, vec2(cos_omega_kt,  sin_omega_kt)) +
                        complexMultiply(fourierData.zw, vec2(cos_omega_kt, -sin_omega_kt));

    // Height
    imageStore(imgOcean, ivec3(pix, LAYER_HEIGHT), vec4(hTilde, 0, 0));

    // Slope
    imageStore(imgOcean, ivec3(pix, LAYER_SLOPE), vec4(
        complexMultiply(hTilde, vec2(0.0, kData.x)),
        complexMultiply(hTilde, vec2(0.0, kData.y))
    ));

    // Differentials
    if (kData.z < EPSILON) {
        imageStore(imgOcean, ivec3(pix, LAYER_DIFFERENTIAL), vec4(0,0,0,0));
    } else {
        imageStore(imgOcean, ivec3(pix, LAYER_DIFFERENTIAL), vec4(
            complexMultiply(hTilde, vec2(0.0, -kData.x / kData.z)),
            complexMultiply(hTilde, vec2(0.0, -kData.y / kData.z))
        ));
    }
}