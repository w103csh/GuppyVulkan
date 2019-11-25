
#version 450

#define _DS_HFF_CLMN 0

// BINDINGS
layout(set=_DS_HFF_CLMN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_HFF_CLMN, binding=2) uniform Matrix4 {
    mat4 model;
} matrix4;
layout(set=_DS_HFF_CLMN, binding=3) uniform Simulation {
    float c2;        // wave speed
    float h;         // column width/height
    float h2;        // h squared
    float dt;        // time delta
    float maxSlope;  // clamped sloped to prevent numerical explosion
    int read, write;
    int mMinus1, nMinus1;
} sim;
layout(set=_DS_HFF_CLMN, binding=4, r32f) uniform readonly image3D imgHeightField;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in ivec2 inImageOffset;

// OUT
layout(location=0) out vec3 outPosition;        // (camera space)
layout(location=1) out vec3 outNormal;          // (camera space)
layout(location=2) out vec4 outColor;
layout(location=3) out flat uint outFlags;

void main() {
    mat4 mViewModel = camera.view * matrix4.model;

    outPosition = inPosition;

    ivec3 size = imageSize(imgHeightField);

    const float _h = sim.h * 0.5f; 

    float _u = 0.0f;
    float u = imageLoad(imgHeightField, ivec3(inImageOffset, sim.write)).r;
    u = max(u, 0.02f);

    if (gl_VertexIndex == 0
        || gl_VertexIndex == 19
        || gl_VertexIndex == 23
        || gl_VertexIndex == 32
        || gl_VertexIndex == 34
    ) {
        // front ll, left lr, bottom ul
        outPosition.x += -_h;
        outPosition.z += _h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 1
        || gl_VertexIndex == 5
        || gl_VertexIndex == 6
        || gl_VertexIndex == 33
    ) {
        // front lr, right ll, bottom ur
        outPosition.x += _h;
        outPosition.z += _h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 2
        || gl_VertexIndex == 4
        || gl_VertexIndex == 21
        || gl_VertexIndex == 24
    ) {
        // front ul, left ur, top ll
        outPosition.x += -_h;
        outPosition.z += _h;
        outPosition.y += u;
    } else if (gl_VertexIndex == 3
        || gl_VertexIndex == 8
        || gl_VertexIndex == 10
        || gl_VertexIndex == 25
        || gl_VertexIndex == 29
    ) {
        // front ur, right ul, top lr
        outPosition.x += _h;
        outPosition.z += _h;
        outPosition.y += u;
    } else if (gl_VertexIndex == 7
        || gl_VertexIndex == 11
        || gl_VertexIndex == 12
        || gl_VertexIndex == 31
        || gl_VertexIndex == 35
    ) {
        // right lr, back ll, bottom lr
        outPosition.x += _h;
        outPosition.z += -_h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 9
        || gl_VertexIndex == 14
        || gl_VertexIndex == 16
        || gl_VertexIndex == 27
    ) {
        // right ur, back ul, top ur
        outPosition.x += _h;
        outPosition.z += -_h;
        outPosition.y += u;
    } else if (gl_VertexIndex == 13
        || gl_VertexIndex == 17
        || gl_VertexIndex == 18
        || gl_VertexIndex == 30
    ) {
        // back lr, left ll, bottom ll
        outPosition.x += -_h;
        outPosition.z += -_h;
        outPosition.y += _u;
    } else if (gl_VertexIndex == 15
        || gl_VertexIndex == 20
        || gl_VertexIndex == 22
        || gl_VertexIndex == 26
        || gl_VertexIndex == 28
    ) {
        // back ur, left ul, top ul
        outPosition.x += -_h;
        outPosition.z += -_h;
        outPosition.y += u;
    }

    if (gl_VertexIndex < 6) {
        outNormal = vec3(0.0, 0.0, 1.0); // front
    } else if (gl_VertexIndex < 12) {
        outNormal = vec3(1.0, 0.0, 0.0); // right
    } else if (gl_VertexIndex < 18) {
        outNormal = vec3(0.0, 0.0, -1.0); // back
    } else if (gl_VertexIndex < 24) {
        outNormal = vec3(-1.0, 0.0, 0.0); // left
    } else if (gl_VertexIndex < 30) {
        outNormal = vec3(0.0, 1.0, 0.0); // top
    } else {
        outNormal = vec3(0.0, -1.0, 0.0); // bottom
    }

    outPosition = (mViewModel * vec4(outPosition, 1.0)).xyz;
    outNormal = mat3(mViewModel) * outNormal;
    outFlags = 0x0u;
    outColor = vec4(1,0,0,1);

    gl_Position = camera.projection * vec4(outPosition, 1.0);
}