/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DEFCAM_DEFMAT_MX4 0

// DECLARATIONS
float getLifespan();
float getMinParticleSize();
float getMaxParticleSize();
bool  isMesh();

// FLAGS
const uint SMOKE        = 0x00000004u;

// PUSH CONSTANTS
layout(push_constant) uniform PushConstantsBlock {
    uint flags;
} pushConstants;

// BINDINGS
layout(set=_DS_UNI_DEFCAM_DEFMAT_MX4, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_UNI_DEFCAM_DEFMAT_MX4, binding=2) uniform Matrix4 {
    mat4 model;
} uniMat4;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
/**
 * data0[0]: position.x
 * data0[1]: position.y
 * data0[2]: position.z
 * data0[3]: padding
 *
 * data1[0]: veloctiy.x
 * data1[1]: veloctiy.y
 * data1[2]: veloctiy.z
 * data1[3]: age
 *
 * data2[0]: rotation angle
 * data2[1]: rotation velocity
 * data2[2]: padding
 * data2[3]: padding
 */
layout(location=3) in vec4 inData0;
layout(location=4) in vec4 inData1;
layout(location=5) in vec4 inData2;

// OUT
layout(location=0) out vec3 outPosition;        // (world space meshes, camera space quads)
layout(location=1) out vec3 outNormal;          // (world space meshes, camera space quads)
layout(location=3) out vec2 outTexCoord;
layout(location=4) out float outTransparency;
layout(location=5) out flat uint outFlags;

// Offsets to the position in camera coordinates for each vertex of the particle's quad
const vec3 offsets[] = vec3[](vec3(-0.5, -0.5, 0), vec3(0.5, -0.5, 0), vec3( 0.5, 0.5, 0),
                              vec3(-0.5, -0.5, 0), vec3(0.5,  0.5, 0), vec3(-0.5, 0.5, 0));
// Texture coordinates for each vertex of the particle's quad
const vec2 texCoords[] = vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(0, 1));

void main() {
    // Mesh
    if (isMesh()) {

        float cs = cos(inData2.x);
        float sn = sin(inData2.x);
        mat4 mRotAndTrans = uniMat4.model * mat4(
            1, 0, 0, 0,
            0, cs, sn, 0,
            0, -sn, cs, 0,
            inData0.x, inData0.y, inData0.z, 1
        );

        if (inData1[3] >= 0.0) {
            outPosition = (mRotAndTrans * vec4(inPosition, 1.0)).xyz;
        } else {
            outPosition = vec3(0.0);
        }
        outTransparency = 1.0;
        outTexCoord = vec2(0.0);
        outNormal = normalize(mat3(mRotAndTrans) * inNormal);
        outFlags = 0x00u;

        gl_Position = camera.viewProjection * vec4(outPosition, 1.0);

    // Billboard-ed quad
    } else {
        mat4 mViewModel = camera.view * uniMat4.model;

        float agePct = inData1[3] / getLifespan();

        float particleSize;
        if ((pushConstants.flags & SMOKE) > 0) {
            particleSize = mix(getMinParticleSize(), getMaxParticleSize(), agePct);
        } else {
            particleSize = getMinParticleSize();
        }

        if (inData1[3] >= 0.0) {
            outPosition = (mViewModel * vec4(inData0.xyz, 1.0)).xyz +
                (offsets[gl_VertexIndex] * particleSize);       // camera space
            outTransparency = clamp(1.0 - agePct, 0.0, 1.0);
        } else {
            outPosition = vec3(0.0);
        }
        outTexCoord = texCoords[gl_VertexIndex];
        outNormal = vec3(0.0, 0.0, 1.0);                        // camera space
        outFlags = 0x02u;                                       // camera space

        gl_Position = camera.projection * vec4(outPosition, 1.0);
    }
}