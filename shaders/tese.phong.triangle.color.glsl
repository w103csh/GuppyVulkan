/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DFR_MRT 0
#define PATCH_CONTROL_POINTS 3

#define interp3(u, v, w, t)  u * t.x + v * t.y + w * t.z
#define interp3Array(uvw, t)  uvw[0] * t.x + uvw[1] * t.y + uvw[2] * t.z

layout(set=_DS_UNI_DFR_MRT, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
 // TODO: add to material or uniform
const float alpha = 3.0 / 4.0;

struct PhongPatch {
    float termIJ;
    float termJK;
    float termKI;
    float di;
};

// IN
 // Using "cw" here and the interp3d functions below I can use the typical primitive windin
 // for the "frontFace" (counter-clockwise) setting the rasterization state info.
layout(triangles, equal_spacing, cw) in;
layout(location=PATCH_CONTROL_POINTS*0) in vec3 inNormal[];
layout(location=PATCH_CONTROL_POINTS*1) in vec4 inColor[];
layout(location=PATCH_CONTROL_POINTS*2) in PhongPatch inPhongPatch[];
// OUT
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;

#define Pi  gl_in[0].gl_Position.xyz
#define Pj  gl_in[1].gl_Position.xyz
#define Pk  gl_in[2].gl_Position.xyz

void main() {
    // Precompute squared tesscoords
    const vec3 tessCoordSquared = gl_TessCoord * gl_TessCoord;

    // Interpolated position
    const vec3 p = interp3(Pi, Pj, Pk, gl_TessCoord);

    // Build terms
    const vec3 termIJ = vec3(inPhongPatch[0].termIJ, inPhongPatch[1].termIJ, inPhongPatch[2].termIJ);
    const vec3 termJK = vec3(inPhongPatch[0].termJK, inPhongPatch[1].termJK, inPhongPatch[2].termJK);
    const vec3 termKI = vec3(inPhongPatch[0].termKI, inPhongPatch[1].termKI, inPhongPatch[2].termKI);

    // Phong tesselated position
    vec3 pStar = tessCoordSquared.x * Pi
               + tessCoordSquared.y * Pj
               + tessCoordSquared.z * Pk
               + gl_TessCoord.x * gl_TessCoord.y * termIJ
               + gl_TessCoord.y * gl_TessCoord.z * termJK
               + gl_TessCoord.z * gl_TessCoord.x * termKI;

    // Depth measurement factor
    float d_alpha = alpha;
    if (false) {
        // This needs more thought. See the comment in the control shader.
        float d = inPhongPatch[0].di * gl_TessCoord.x
                + inPhongPatch[1].di * gl_TessCoord.y
                + inPhongPatch[2].di * gl_TessCoord.z;
        d_alpha = d * alpha;
    }

    // position
    outPosition = (1.0 - d_alpha) * p + d_alpha * pStar;
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // normal
    outNormal = interp3Array(inNormal, gl_TessCoord);
    // color
    outColor = interp3Array(inColor, gl_TessCoord);
    // tex coordinate
    // outTexCoord = interp3_2(inTexCoord[0], inTexCoord[1], inTexCoord[2]);
}
