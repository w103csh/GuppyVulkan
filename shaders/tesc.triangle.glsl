/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// UNIFORMS
#define _DS_UNI_TESS_DEF 0

// layout(constant_id = 0) const int PATCH_CONTROL_POINTS = 6;
#define PATCH_CONTROL_POINTS 3

layout(set=_DS_UNI_TESS_DEF, binding=0) uniform Default {
    ivec4 outerLevel;
    ivec2 innerLevel;
} uniDefault;

// IN
layout(location=0) in vec3 inNormal[];
layout(location=1) in vec4 inColor[];
// OUT
layout(vertices=PATCH_CONTROL_POINTS) out;
layout(location=PATCH_CONTROL_POINTS*0) out vec3 outNormal[PATCH_CONTROL_POINTS];
layout(location=PATCH_CONTROL_POINTS*1) out vec4 outColor[PATCH_CONTROL_POINTS];

void main() {
    // Pass along the vertex position unmodified
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
	outColor[gl_InvocationID] = inColor[gl_InvocationID];

    gl_TessLevelOuter[0] = float(uniDefault.outerLevel[0]);
    gl_TessLevelOuter[1] = float(uniDefault.outerLevel[1]);
    gl_TessLevelOuter[2] = float(uniDefault.outerLevel[2]);

    gl_TessLevelInner[0] = float(uniDefault.innerLevel[0]);
}
