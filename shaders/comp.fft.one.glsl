/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_FFT 0

// BINDINGS
layout(set=_DS_FFT, binding=0, rg32f  ) uniform image2DArray imgFFT;
// IN
layout(local_size_x=1, local_size_y=1) in;

const float PI = 3.14159265358979323846;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const int m = 2, dir = 1;

/**
 *  This algorithm comes from here: http://paulbourke.net/miscellaneous/dft/. It was
 *  modified slightly but the idea is the same. The entire image is transformed using
 *  compute work groups.
 */
void rows(const in int col) {
    int n, i, i1, j, k, i2, l, l1, l2;
    float c1, c2, u1, u2, z;
    vec2 t1, t2;

    /* Calculate the number of points */
    n = imageSize(imgFFT).x;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            t1 = imageLoad(imgFFT, ivec3(i, col, 0)).xy;
            imageStore(imgFFT,
                ivec3(i, col, 0),
                vec4(imageLoad(imgFFT, ivec3(j, col, 0)).xy, 0, 0)
            );
            imageStore(imgFFT,
                ivec3(j, col, 0),
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
    for (l = 0; l < m; l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j = 0; j < l1; j++) {
            for (i = j; i < n; i += l2) {
                i1 = i + l1;
                t1 = imageLoad(imgFFT, ivec3(i1, col, 0)).xy;
                t1 = vec2(
                    u1 * t1.x - u2 * t1.y,
                    u1 * t1.y + u2 * t1.x
                );
                t2 = imageLoad(imgFFT, ivec3(i, col, 0)).xy;
                imageStore(imgFFT,
                    ivec3(i1, col, 0),
                    vec4(t2 - t1, 0, 0)
                );
                imageStore(imgFFT,
                    ivec3(i, col, 0),
                    vec4(t2 + t1, 0, 0)
                );
            }
            z = u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        if (dir == 1) // necessary ???
            c2 = -c2;
        c1 = sqrt((1.0 + c1) / 2.0);
    }

}

/**
 *  This algorithm comes from here: http://paulbourke.net/miscellaneous/dft/. It was
 *  modified slightly but the idea is the same. The entire image is transform using
 *  compute work groups.
 */
void cols(const in int row) {
    int n, i, i1, j, k, i2, l, l1, l2;
    float c1, c2, u1, u2, z;
    vec2 t1, t2;

    /* Calculate the number of points */
    n = imageSize(imgFFT).y;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            t1 = imageLoad(imgFFT, ivec3(row, i, 0)).xy;
            imageStore(imgFFT,
                ivec3(row, i, 0),
                vec4(imageLoad(imgFFT, ivec3(row, j, 0)).xy, 0, 0)
            );
            imageStore(imgFFT,
                ivec3(row, j, 0),
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
    for (l = 0; l < m; l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j = 0; j < l1; j++) {
            for (i = j; i < n; i += l2) {
                i1 = i + l1;
                t1 = imageLoad(imgFFT, ivec3(row, i1, 0)).xy;
                t1 = vec2(
                    u1 * t1.x - u2 * t1.y,
                    u1 * t1.y + u2 * t1.x
                );
                t2 = imageLoad(imgFFT, ivec3(row, i, 0)).xy;
                imageStore(imgFFT,
                    ivec3(row, i1, 0),
                    vec4(t2 - t1, 0, 0)
                );
                imageStore(imgFFT,
                    ivec3(row, i, 0),
                    vec4(t2 + t1, 0, 0)
                );
            }
            z = u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        if (dir == 1) // necessary ???
            c2 = -c2;
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