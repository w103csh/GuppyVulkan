
#version 450

#define _DS_UNI_PRTCL_FNTN 0

// FLAGS
const uint IS_MESH                  = 0x00010000u;

// BINDINGS
layout(set=_DS_UNI_PRTCL_FNTN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

layout(set=_DS_UNI_PRTCL_FNTN, binding=1) uniform ParticleFountain {
    // Material::Base::DATA
    vec3 color;             // Diffuse color for dielectrics, f0 for metallic
    float opacity;          // Overall opacity
    // 16
    uint flags;             // Flags (general/material)
    uint texFlags;          // Flags (texture)
    float xRepeat;          // Texture xRepeat
    float yRepeat;          // Texture yRepeat

    // Material::Obj3d::DATA
    mat4 model;

    // Material::Particle::Fountain::DATA
    // 0, 1, 2 : Particle acceleration (gravity)
    // 4 :       Particle lifespan
    vec4 data0;
    // 0, 1, 2 : World position of the emitter.
    // 4 :       Size of particle
    vec4 data1;
    mat4 emitterBasis;      // Rotation that rotates y axis to the direction of emitter
    float time;             // Simulation time
    float delta;            // Elapsed time between frames
} uniFountain;

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
 * data2[1]: veloctiy.y
 * data3[2]: veloctiy.z
 * data4[3]: age
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
layout(location=0) out vec3 outPosition;        // camera space
layout(location=1) out vec3 outNormal;          // camera space
layout(location=3) out vec2 outTexCoord;
layout(location=4) out float outTransparency;

// Offsets to the position in camera coordinates for each vertex of the particle's quad
const vec3 offsets[] = vec3[](vec3(-0.5, -0.5, 0), vec3(0.5, -0.5, 0), vec3( 0.5, 0.5, 0),
                              vec3(-0.5, -0.5, 0), vec3(0.5,  0.5, 0), vec3(-0.5, 0.5, 0));
// Texture coordinates for each vertex of the particle's quad
const vec2 texCoords[] = vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(0, 1));

void main() {
    mat4 mViewModel = camera.view * uniFountain.model;

    // Mesh
    if ((uniFountain.flags & IS_MESH) > 0) {

        float cs = cos(inData2.x);
        float sn = sin(inData2.x);
        mat4 mRotAndTrans = mat4(
            1, 0, 0, 0,
            0, cs, sn, 0,
            0, -sn, cs, 0,
            inData0.x, inData0.y, inData0.z, 1
        );
        mViewModel = mViewModel * mRotAndTrans;

        if (inData1[3] >= 0.0) {
            outPosition = (mViewModel * vec4(inPosition, 1.0)).xyz;
            outTransparency = clamp(1.0 - inData1[3] / uniFountain.data0[3], 0.0, 1.0);
        } else {
            outPosition = vec3(0.0);
        }
        outTexCoord = vec2(0.0);

    // Billboarded quad
    } else {

        if (inData1[3] >= 0.0) {
            outPosition = (mViewModel * vec4(inData0.xyz, 1.0)).xyz +
                (offsets[gl_VertexIndex] * uniFountain.data1[3]);
            outTransparency = clamp(1.0 - inData1[3] / uniFountain.data0[3], 0.0, 1.0);
        } else {
            outPosition = vec3(0.0);
        }
        outTexCoord = texCoords[gl_VertexIndex];
    }

    outNormal = normalize(mat3(mViewModel) * inNormal);

    gl_Position = camera.projection * vec4(outPosition, 1.0);
}