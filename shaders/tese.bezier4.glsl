
#version 450

// UNIFORMS
#define _DS_UNI_BEZ 0

#define CONTROL_POINTS 4

layout(set=_DS_UNI_BEZ, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(isolines) in;
layout(location=CONTROL_POINTS*0) in vec3 inNormal[];
layout(location=CONTROL_POINTS*1) in vec4 inColor[];
// OUT
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;

void main()
{
    float u = gl_TessCoord.x;

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    float u1 = (1.0 - u);
    float u2 = u * u;

    // Bernstein polynomials
    float b3 = u2 * u;
    float b2 = 3.0 * u2 * u1;
    float b1 = 3.0 * u * u1 * u1;
    float b0 = u1 * u1 * u1;

    // Cubic Bezier interpolation
    vec3 p = p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;

    gl_Position = camera.viewProjection * vec4(p, 1.0);

    outPosition = p;
    // outNormal = gl_TessCoord[2]*inNormal[0] + gl_TessCoord[0]*inNormal[1] + gl_TessCoord[1]*inNormal[2];
    outNormal = inNormal[0];
    outColor = inColor[0];
}
