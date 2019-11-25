
#version 450

// UNIFORMS
#define _DS_UNI_DFR_MRT 0

// layout(constant_id = 0) const int PATCH_CONTROL_POINTS = 6;
#define PATCH_CONTROL_POINTS 3

layout(set=_DS_UNI_DFR_MRT, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
// Using cw here. Maybe if the interpolation algorithm expects the inputs to be
// in cw order. Then, because the pipelines are all ccw they have to be
// reversed here. This is just a guess though. It just makes more sense to me
// to multiply x*0, y*1, and z*2, so I am setting cw here.
layout(triangles, cw) in; 
layout(location=PATCH_CONTROL_POINTS*0) in vec3 inNormal[];
layout(location=PATCH_CONTROL_POINTS*1) in vec4 inColor[];
// OUT
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;
layout(location=3) out flat uint outFlags;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
    return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void bernsteinBasisFunctions(const in float t, out float[3] b, out float[3] db) {
    // [(1-t) + t]^n
    float tmin1 = (1.0 - t);

    b[0] = tmin1 * tmin1;
    b[1] = 2.0 * t * tmin1;
    b[2] = t * t;

    db[0] = -2.0 * tmin1;
    db[1] = (2.0 * tmin1) - (2.0 * t);
    db[2] = 2.0 * t;
}

void main() {

    // This what I was doing is not going to work right. I need to read about cubic
    // bezier triangles. Also the derivatives need to be parital derivaties with
    // respect to u & v.
    bool tessellateSurface = PATCH_CONTROL_POINTS > 3;

    if (tessellateSurface) {
        // Reassign
        float u = gl_TessCoord.x;
        float v = gl_TessCoord.y;
        vec3 p00 = gl_in[0].gl_Position.xyz;
        vec3 p01 = gl_in[1].gl_Position.xyz;
        vec3 p02 = gl_in[2].gl_Position.xyz;
#if (PATCH_CONTROL_POINTS > 5)
        vec3 p10 = gl_in[3].gl_Position.xyz;
        vec3 p11 = gl_in[4].gl_Position.xyz;
        vec3 p12 = gl_in[5].gl_Position.xyz;
#endif

        // Compute basis functions
        float bu[3], bv[3];   // Basis functions for u and v
        float dbu[3], dbv[3]; // Derivitives for u and v
        bernsteinBasisFunctions(u, bu, dbu);
        bernsteinBasisFunctions(v, bv, dbv);

        // Bezier interpolation
        outPosition = p00*bu[0]*bv[0] + p01*bu[0]*bv[1] + p02*bu[0]*bv[2];
#if (PATCH_CONTROL_POINTS > 5)
        outPosition += p10*bu[1]*bv[0] + p11*bu[1]*bv[1] + p12*bu[1]*bv[2];
#endif

        // Compute the normal vector
        vec3 du = p00*dbu[0]*bv[0] + p01*dbu[0]*bv[1] + p02*dbu[0]*bv[2];
#if (PATCH_CONTROL_POINTS > 5)
        du += p10*dbu[1]*bv[0] + p11*dbu[1]*bv[1] + p12*dbu[1]*bv[2];
#endif

        vec3 dv = p00*bu[0]*dbv[0] + p01*bu[0]*dbv[1] + p02*bu[0]*dbv[2];
#if (PATCH_CONTROL_POINTS > 5)
        dv += p10*bu[1]*dbv[0] + p11*bu[1]*dbv[1] + p12*bu[1]*dbv[2];
#endif

        gl_Position = camera.viewProjection * vec4(outPosition, 1.0);

        // outNormal = normalize(cross(du, dv));
        outNormal = vec3(u, v, 0.25);
        // outColor = vec4(interpolate3D(inColor[0].xyz, inColor[1].xyz, inColor[2].xyz), 1.0);
        outColor = vec4(interpolate3D(inColor[0].xyz, inColor[1].xyz, inColor[2].xyz), 1.0);

    } else {

        vec3 p = interpolate3D(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
        p += interpolate3D(gl_in[3].gl_Position.xyz, gl_in[4].gl_Position.xyz, gl_in[5].gl_Position.xyz);
        gl_Position = camera.viewProjection * vec4(p, 1.0);
        outPosition = p;
        outNormal = interpolate3D(inNormal[0], inNormal[1], inNormal[2]);
        outColor = vec4(interpolate3D(inColor[0].xyz, inColor[1].xyz, inColor[2].xyz), 1.0);

    }

    outFlags = 0x0u;
}
