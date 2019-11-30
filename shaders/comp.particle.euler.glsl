/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_PRTCL_EULER 0

// DECLARATIONS
vec3  getAcceleration();
float getLifespan();
vec3  getEmitterPosition();
mat4  getEmitterBasis();
float getDelta();
float getVelocityLowerBound();
float getVelocityUpperBound();

const float PI = 3.14159265359;

// FLAGS
const uint DEFAULT              = 0x00000001u;
const uint FIRE                 = 0x00000002u;
const uint SMOKE                = 0x00000004u;

// PUSH CONSTANTS
layout(push_constant) uniform PushConstantsBlock {
    uint flags;
} pushConstants;

// SAMPLERS
layout(set=_DS_PRTCL_EULER, binding=0) uniform sampler1D sampRandom;

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

// STORAGE BUFFERS
layout(set=_DS_PRTCL_EULER, binding=1, std140) buffer ParticleBuffer {
    Particle particles[];
};

vec3 randomInitialVelocity() {
    float velocity; vec3 v;
    int index = int(gl_GlobalInvocationID.x);
    if ((pushConstants.flags & FIRE) > 0) {
        velocity = mix(0.1, 0.5, texelFetch(sampRandom, index + 1, 0).r);
        v = vec3(0, 1, 0);
    } else if ((pushConstants.flags & SMOKE) > 0) {
        float theta = mix(0.0, PI / 1.5, texelFetch(sampRandom, index, 0).r);
        float phi = mix(0.0, 2.0 * PI, texelFetch(sampRandom, index + 1, 0).r);
        velocity = mix(0.1, 0.2, texelFetch(sampRandom, index + 2, 0).r);
        v = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
    } else {
        float theta = mix(0.0, PI / 8.0, texelFetch(sampRandom, index, 0).r);
        float phi = mix(0.0, 2.0 * PI, texelFetch(sampRandom, index + 1, 0).r);
        velocity = mix(getVelocityLowerBound(),
            getVelocityUpperBound(),
            texelFetch(sampRandom, index + 2, 0).r);
        v = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
    }
    v = normalize(mat3(getEmitterBasis()) * v) * velocity;
    return v;
}

vec3 randomInitialPosition() {
    vec3 p;
    if ((pushConstants.flags & FIRE) > 0) {
        float offset = mix(-2.0, 2.0, texelFetch(sampRandom, int(gl_GlobalInvocationID.x), 0).r);
        p = getAcceleration() + vec3(offset, 0, 0);
    } else {
        p = getEmitterPosition();
    }
    return p;
}

float randomInitialRotationalVelocity() {
    return mix(-15.0, 15.0, texelFetch(sampRandom, int(gl_GlobalInvocationID.x) + 3, 0).r);
}

// LOCAL SIZE
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

void main() {
    uint index = gl_GlobalInvocationID.x;
    if (particles[index].data1[3] < 0 || particles[index].data1[3] > getLifespan()) {
        // The particle is past it's lifetime, recycle.
        particles[index].data0.xyz = randomInitialPosition();                       // position
        particles[index].data1.xyz = randomInitialVelocity();                       // veloctiy
        particles[index].data2.xy = vec2(0.0, randomInitialRotationalVelocity());   // rotation
        if (particles[index].data1[3] < 0)                                          // age
            particles[index].data1[3] += getDelta();
        else
            particles[index].data1[3] += -getLifespan() + getDelta();
    } else {
        particles[index].data0.xyz += particles[index].data1.xyz * getDelta();      // position
        particles[index].data1.xyz += getAcceleration() * getDelta();               // velocity
        particles[index].data2.x = mod(particles[index].data2.x +                   // rotation
            particles[index].data2.y * getDelta(), 2.0 * PI);
        particles[index].data1[3] += getDelta();                                    // age
    }
}