
#version 450

#define _DS_PRTCL_EULER 0

struct Particle {
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
    vec4 data0;
    vec4 data1;
    vec4 data2;
};

const float PI = 3.14159265359;

// // SPECIALIZATION CONSTANTS (This doesn't work for local size unfortunately)
// layout (constant_id = 0) const uint _LOCAL_SIZE_X = 1000;

// // PUSH CONSTANTS
// layout(push_constant) uniform PushConstantsBlock {
//     uint readOffset;
//     uint writeOffset;
// } pushConstants;

// UNIFORMS
layout(set=_DS_PRTCL_EULER, binding=0) uniform ParticleFountain {
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
    float velLB;            // Lower bound of the generated random velocity (euler)
    float velUB;            // Upper bound of the generated random velocity (euler)
} uniFountain;

// SAMPLERS
layout(set=_DS_PRTCL_EULER, binding=1) uniform sampler1D sampRandom;

// STORAGE BUFFERS
layout(set=_DS_PRTCL_EULER, binding=2, std140) buffer ParticleBuffer {
    Particle particles[];
};

// IN
layout(local_size_x=16, local_size_y=1, local_size_z=1) in;

vec3 randomInitialVelocity() {
    int index = int(gl_GlobalInvocationID.x);
    float theta = mix(0.0, PI / 8.0, texelFetch(sampRandom, index, 0).r);
    float phi = mix(0.0, 2.0 * PI, texelFetch(sampRandom, index + 1, 0).r);
    float velocity = mix(uniFountain.velLB, uniFountain.velUB, texelFetch(sampRandom, index + 2, 0).r);
    vec3 v = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
    return normalize(mat3(uniFountain.emitterBasis) * v) * velocity;
}

float randomInitialRotationalVelocity() {
    return mix(-15.0, 15.0, texelFetch(sampRandom, int(gl_GlobalInvocationID.x) + 3, 0).r);
}

void main() {
    uint index = gl_GlobalInvocationID.x;
    if (particles[index].data1[3] < 0 || particles[index].data1[3] > uniFountain.data0[3]) {
        // The particle is past it's lifetime, recycle.
        particles[index].data0.xyz = uniFountain.data1.xyz;                             // position
        particles[index].data1.xyz = randomInitialVelocity();                           // veloctiy
        particles[index].data2.xy = vec2(0.0, randomInitialRotationalVelocity());       // rotation
        if (particles[index].data1[3] < 0)                                              // age
            particles[index].data1[3] += uniFountain.delta;
        else
            particles[index].data1[3] += -uniFountain.data0[3] + uniFountain.delta;
    } else {
        particles[index].data0.xyz += particles[index].data1.xyz * uniFountain.delta;   // position
        particles[index].data1.xyz += uniFountain.data0.xyz * uniFountain.delta;        // velocity
        particles[index].data2.x = mod(particles[index].data2.x +                       // rotation
            particles[index].data2.y * uniFountain.delta, 2.0 * PI);
        particles[index].data1[3] += uniFountain.delta;                                 // age
    }
}