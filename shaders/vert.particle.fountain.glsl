
#version 450

#define _DS_UNI_PRTCL_FNTN 0

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
layout(location=3) in vec3 inVelocity;
layout(location=4) in float inTimeOfBirth;

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

    float t = uniFountain.delta - inTimeOfBirth;
    if (t >= 0 && t < uniFountain.data0[3]) {
        vec3 pos = uniFountain.data1.xyz +
            (inVelocity * t) +
            (uniFountain.data0.xyz * t * t);
        outPosition = (mViewModel * vec4(pos, 1.0)).xyz +
            (offsets[gl_VertexIndex] * uniFountain.data1[3]);
        outTransparency = mix(1.0, 0.0, t / uniFountain.data0[3]);
    } else {
        // Particle doesn't "exist", draw fully transparent
        outPosition = vec3(0.0);
        outTransparency = 0.0;
    }

    outNormal = vec3(0.0, 0.0, 1.0);
    outTexCoord = texCoords[gl_VertexIndex];

    gl_Position = camera.projection * vec4(outPosition, 1.0);
}