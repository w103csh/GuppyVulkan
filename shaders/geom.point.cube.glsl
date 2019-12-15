/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
 #version 450

#define _DS_CAM_CUBE 0

const int CUBE_LAYERS = 6;
layout(set=_DS_CAM_CUBE, binding=0) uniform CameraCube {
    mat4 views[CUBE_LAYERS];
    mat4 proj;
} camera;

// IN
layout(points) in;
layout(location=0) in vec3 inNormal[];
layout(location=1) in vec4 inColor[];
// OUT
layout(points, max_vertices=CUBE_LAYERS) out;
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec4 outColor;

void main() {
    int layer, i;
    for (layer = 0; layer < CUBE_LAYERS; layer++) {
        gl_Layer = layer;
        // Position
        outPosition = gl_in[i].gl_Position.xyz;
        gl_Position = camera.proj * camera.views[layer] * vec4(outPosition, 1.0);
        // Normal
        outNormal = inNormal[i];
        // Color
        outColor = inColor[i];

        EmitVertex();
        EndPrimitive();
    }
}
