
#version 450

// UNIFORMS
#define _DS_UNI_BEZ 0

#define CONTROL_POINTS 4

layout(set=_DS_UNI_BEZ, binding=1) uniform Bezier {
    int strips;
    int segments;
} uniBezier;

// IN
layout(location=0) in vec3 inNormal[];
layout(location=1) in vec4 inColor[];
// OUT
layout(vertices=CONTROL_POINTS) out;
layout(location=CONTROL_POINTS*0) out vec3 outNormal[CONTROL_POINTS];
layout(location=CONTROL_POINTS*1) out vec4 outColor[CONTROL_POINTS];

void main()
{
    // Pass along the vertex position unmodified
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
	outColor[gl_InvocationID] = inColor[gl_InvocationID];

    // Some drivers (e.g. Intel) treat these levels incorrectly.  The OpenGL spec
    // says that level 0 should be the number of strips and level 1 should be
    // the number of segments per strip.  Unfortunately, not all drivers do this.
    // If this example doesn't work for you, try switching the right
    // hand side of the two assignments below.
    gl_TessLevelOuter[0] = float(uniBezier.strips);
    gl_TessLevelOuter[1] = float(uniBezier.segments);
}
