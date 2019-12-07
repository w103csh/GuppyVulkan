/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
 #version 450

#define _DS_SHDW_CUBE_UNI_ONLY 0
#define _U_LGT_SHDW_CUBE 0

// BINDINGS
const int CUBE_LAYERS = 6;
layout(set=_DS_SHDW_CUBE_UNI_ONLY, binding=0) uniform LightShadowCube {
    mat4 viewProj[CUBE_LAYERS]; // cube map mvps
    vec4 data0;                 // xyz: world position, w: cutoff
    vec3 cameraPosition;        //
    uint flags;                 //
    vec3 La;                    // Ambient intensity
    vec3 L;                     // Diffuse and specular intensity
} lgtShdwCube[_U_LGT_SHDW_CUBE];

// IN
layout(triangles) in;
// OUT
layout(triangle_strip, max_vertices=3*CUBE_LAYERS*_U_LGT_SHDW_CUBE) out;

void main() {
    int light, lightOffset, layer, i;
    for (light = 0; light < _U_LGT_SHDW_CUBE; light++) {
        lightOffset = light * CUBE_LAYERS;
        for (layer = 0; layer < CUBE_LAYERS; layer++) {
            gl_Layer = lightOffset + layer;
            for (i = 0; i < gl_in.length(); i++) {
                gl_Position = lgtShdwCube[light].viewProj[layer] * vec4(gl_in[i].gl_Position.xyz, 1.0);
                EmitVertex();
            }
            EndPrimitive();
        }
    }
}
