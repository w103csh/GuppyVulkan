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
layout(location=0) in vec3 inNormal[];      // (world space)
layout(location=1) in vec2 inTexCoord[];
layout(location=2) in vec3 inTangent[];     // (world space)
layout(location=3) in vec3 inBinormal[];    // (world space)
// OUT
layout(triangle_strip, max_vertices=3*CUBE_LAYERS) out;
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out flat vec2 outTexCoord;
layout(location=3) out mat3 outTBN;

void main() {
    int layer, i;
    for (layer = 0; layer < CUBE_LAYERS; layer++) {
        gl_Layer = layer;
        for (i = 0; i < gl_in.length(); i++) {
            // Position
            outPosition = gl_in[i].gl_Position.xyz;
            gl_Position = camera.proj * camera.views[layer] * vec4(outPosition, 1.0);
            // Normal
            outNormal = inNormal[i];
            // Camera space to tangent space
            mat3 view = mat3(camera.views[layer]);
            vec3 normal = normalize(view * inNormal[i]);
            vec3 tangent = normalize(view * inTangent[i]);
            vec3 binormal = normalize(view * inBinormal[i]);
            outTBN = transpose(mat3(binormal, tangent, outNormal));
            // Texture coordinate
            outTexCoord = inTexCoord[i];

            // if (layer == 0) {
            //     outNormal = vec3(1, 0, 0);
            // } else if (layer == 1) {
            //     outNormal = vec3(1, 1, 0);
            // } else if (layer == 2) {
            //     outNormal = vec3(1, 1, 1);
            // } else if (layer == 3) {
            //     outNormal = vec3(1, 0, 1);
            // } else if (layer == 4) {
            //     outNormal = vec3(0, 1, 0);
            // } else if (layer == 5) {
            //     outNormal = vec3(0, 0, 1);
            // } else {
            //     outNormal = vec3(0.0);
            // }

            EmitVertex();
        }
        EndPrimitive();
    }
}
