/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
 #version 450

#define _DS_CAM_CUBE 0

// BINDINGS
const int CUBE_LAYERS = 6;
layout(set=_DS_CAM_CUBE, binding=0) uniform CameraCube {
    mat4 views[CUBE_LAYERS];
    mat4 proj;
} camera;

// IN
layout(triangles) in;
layout(location=1) in vec3 inNormal[];
layout(location=2) in vec4 inColor[];
// OUT
layout(triangle_strip, max_vertices=3*CUBE_LAYERS) out;
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;

void main() {
    int layer, i;
    for (layer = 0; layer < CUBE_LAYERS; layer++) {
        gl_Layer = layer;
        for (i = 0; i < gl_in.length(); i++) {

            outPosition = (camera.views[layer] * vec4(gl_in[i].gl_Position.xyz, 1.0)).xyz;
            gl_Position = camera.proj * vec4(outPosition, 1.0);

            outNormal = normalize(mat3(camera.views[layer]) * inNormal[i]);
            
            outColor = inColor[i];

            EmitVertex();
        }
        EndPrimitive();
    }
}
