/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_OCEAN 0

// DECLARATIONS
vec2 complexMultiply(const in vec2 a, const in vec2 b);
float complexMagnitude(const in vec2 a);

// BINDINGS
layout(set=_DS_OCEAN, binding=2) uniform Simulation {
    uvec2 nmLog2;   // log2 of discrete dimensions
    float t;        // time
} sim;
layout(set=_DS_OCEAN, binding=3, rgba32f) uniform image2DArray imgOcean;
// IN
layout(local_size_x=1, local_size_y=1) in;

const float PI = 3.14159265358979323846;
const int LAYER_HEIGHT          = 2;
const int LAYER_SLOPE           = 3;
const int LAYER_DIFFERENTIAL    = 4;

/**
 *  This algorithm comes from here: http://paulbourke.net/miscellaneous/dft/. It was
 *  modified slightly but the idea is the same. The entire image is transform using
 *  compute work groups. This is the IFFT version.
 */
void rows(const in int col) {
    int n, i, i1, j, k, i2, l, l1, l2;
    float c1, c2, u1, u2, z;
    vec2 t1, t2;

    /* Calculate the number of points */
    n = imageSize(imgOcean).x;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            t1 = imageLoad(imgOcean, ivec3(i, col, LAYER_HEIGHT)).xy;
            imageStore(imgOcean, ivec3(i, col, LAYER_HEIGHT),
                vec4(imageLoad(imgOcean, ivec3(j, col, LAYER_HEIGHT)).xy, 0, 0)
            );
            imageStore(imgOcean, ivec3(j, col, LAYER_HEIGHT),
                vec4(t1, 0, 0)
            );
        }
        k = i2;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l = 0; l < sim.nmLog2.y; l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j = 0; j < l1; j++) {
            for (i = j; i < n; i += l2) {
                i1 = i + l1;
                t1 = imageLoad(imgOcean, ivec3(i1, col, LAYER_HEIGHT)).xy;
                t1 = vec2(
                    u1 * t1.x - u2 * t1.y,
                    u1 * t1.y + u2 * t1.x
                );
                t2 = imageLoad(imgOcean, ivec3(i, col, LAYER_HEIGHT)).xy;
                imageStore(imgOcean, ivec3(i1, col, LAYER_HEIGHT),
                    vec4(t2 - t1, 0, 0)
                );
                imageStore(imgOcean, ivec3(i, col, LAYER_HEIGHT),
                    vec4(t2 + t1, 0, 0)
                );
            }
            z = u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        c1 = sqrt((1.0 + c1) / 2.0);
    }

}

/**
 *  This algorithm comes from here: http://paulbourke.net/miscellaneous/dft/. It was
 *  modified slightly but the idea is the same. The entire image is transform using
 *  compute work groups. This is the IFFT version.
 */
void cols(const in int row) {
    int n, i, i1, j, k, i2, l, l1, l2;
    float c1, c2, u1, u2, z;
    vec2 t1, t2;

    /* Calculate the number of points */
    n = imageSize(imgOcean).y;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            t1 = imageLoad(imgOcean, ivec3(row, i, LAYER_HEIGHT)).xy;
            imageStore(imgOcean, ivec3(row, i, LAYER_HEIGHT),
                vec4(imageLoad(imgOcean, ivec3(row, j, LAYER_HEIGHT)).xy, 0, 0)
            );
            imageStore(imgOcean, ivec3(row, j, LAYER_HEIGHT),
                vec4(t1, 0, 0)
            );
        }
        k = i2;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l = 0; l < sim.nmLog2.x; l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j = 0; j < l1; j++) {
            for (i = j; i < n; i += l2) {
                i1 = i + l1;
                t1 = imageLoad(imgOcean, ivec3(row, i1, LAYER_HEIGHT)).xy;
                t1 = vec2(
                    u1 * t1.x - u2 * t1.y,
                    u1 * t1.y + u2 * t1.x
                );
                t2 = imageLoad(imgOcean, ivec3(row, i, LAYER_HEIGHT)).xy;
                imageStore(imgOcean, ivec3(row, i1, LAYER_HEIGHT),
                    vec4(t2 - t1, 0, 0)
                );
                imageStore(imgOcean, ivec3(row, i, LAYER_HEIGHT),
                    vec4(t2 + t1, 0, 0)
                );
            }
            z = u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        c1 = sqrt((1.0 + c1) / 2.0);
    }

}

void main() {
    if (gl_NumWorkGroups.x > gl_NumWorkGroups.y) {
        cols(int(gl_GlobalInvocationID.x));
    } else {
        rows(int(gl_GlobalInvocationID.y));
    }
}