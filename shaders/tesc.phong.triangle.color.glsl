/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DFR_MRT 0

#define PATCH_CONTROL_POINTS 3

layout(set=_DS_UNI_DFR_MRT, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
 // TODO: add to material or uniform
const float m = 32.0; // maximum refinement depth

struct PhongPatch {
    float termIJ;
    float termJK;
    float termKI;
    float di;
};

// IN
layout(location=0) in vec3 inNormal[];
layout(location=1) in vec4 inColor[];
// OUT
layout(vertices=PATCH_CONTROL_POINTS) out;
layout(location=PATCH_CONTROL_POINTS*0) out vec3 outNormal[PATCH_CONTROL_POINTS];
layout(location=PATCH_CONTROL_POINTS*1) out vec4 outColor[PATCH_CONTROL_POINTS];
layout(location=PATCH_CONTROL_POINTS*2) out PhongPatch outPhongPatch[PATCH_CONTROL_POINTS];

#define Pi  gl_in[0].gl_Position.xyz
#define Pj  gl_in[1].gl_Position.xyz
#define Pk  gl_in[2].gl_Position.xyz

// This comes from here: http://onrendering.blogspot.com/2011/12/tessellation-on-gpu-curved-pn-triangles.html
// There must be some kind of special optimization here. I'm not sure I understand why the triangle vertex vi
// only requires accounting for the x component, vj the y, and vk the z. I would imagine it has something to
// do with a property of orthogonal projection. Other than that the implementation is very straightforward
// if you follow the paper.
float PIi(const in int i, const in vec3 q) {
    vec3 q_minus_p = q - gl_in[i].gl_Position.xyz;
    return q[gl_InvocationID] - dot(q_minus_p, inNormal[i]) * inNormal[i][gl_InvocationID];
}

float phongTessAdaptive(const in vec3 p, const in vec3 n) {
    return (1.0 - dot(n, normalize(camera.worldPosition - p))) * m;
}

void main() {
    // Pass along the vertex position unmodified
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    outColor[gl_InvocationID] = inColor[gl_InvocationID];

    // Compute patch data
    outPhongPatch[gl_InvocationID].termIJ = PIi(0,Pj) + PIi(1,Pi);
    outPhongPatch[gl_InvocationID].termJK = PIi(1,Pk) + PIi(2,Pj);
    outPhongPatch[gl_InvocationID].termKI = PIi(2,Pi) + PIi(0,Pk);

    // Tesselate
    if (true) {
        // This needs work. It doesn't account for distance, and it produces
        // T juntions.
        gl_TessLevelOuter[0] = phongTessAdaptive(Pi, inNormal[0]);
        gl_TessLevelOuter[1] = phongTessAdaptive(Pj, inNormal[1]);
        gl_TessLevelOuter[2] = phongTessAdaptive(Pk, inNormal[2]);
        gl_TessLevelInner[0] = (gl_TessLevelOuter[0] +
                                gl_TessLevelOuter[1] +
                                gl_TessLevelOuter[2]) / 3.0;
        outPhongPatch[gl_InvocationID].di = gl_TessLevelOuter[gl_InvocationID] / m;
    } else {
        gl_TessLevelOuter[gl_InvocationID] = m;
        gl_TessLevelInner[0] = m;
    }
}
