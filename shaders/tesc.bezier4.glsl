/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// UNIFORMS
#define _DS_UNI_TESS_DEF 0

// layout(constant_id = 0) const int PATCH_CONTROL_POINTS = 4; // bezier control points
#define PATCH_CONTROL_POINTS 4 // bezier control points

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

    // Some drivers (e.g. Intel) treat these levels incorrectly.  The OpenGL spec
    // says that level 0 should be the number of strips and level 1 should be
    // the number of segments per strip.  Unfortunately, not all drivers do this.
    // If this example doesn't work for you, try switching the right
    // hand side of the two assignments below.
    gl_TessLevelOuter[0] = float(uniDefault.outerLevel[0]);
    gl_TessLevelOuter[1] = float(uniDefault.outerLevel[1]);
}
