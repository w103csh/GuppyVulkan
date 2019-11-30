/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_PRTCL_FNTN 0

layout(set=_DS_UNI_PRTCL_FNTN, binding=0) uniform ParticleFountain {
    // 0, 1, 2 : Particle acceleration (gravity)
    // 3 :       Particle lifespan
    vec4 data0;
    // 0, 1, 2 : World position of the emitter.
    // 3 :       Simulation time (TODO: I don't think this is used for anything)
    vec4 data1;
    mat4 emitterBasis;      // Rotation that rotates y axis to the direction of emitter
    float minParticleSize;  // Minimum size of particle (used as default)
    float maxParticleSize;  // Maximum size of particle
    float delta;            // Elapsed time between frames
    float velLB;            // Lower bound of the generated random velocity (euler)
    float velUB;            // Upper bound of the generated random velocity (euler)
} fountain;

vec3  getAcceleration()         { return fountain.data0.xyz; }
float getLifespan()             { return fountain.data0.w; }
vec3  getEmitterPosition()      { return fountain.data1.xyz; }
float getSimulationTime()       { return fountain.data1.w; }
mat4  getEmitterBasis()         { return fountain.emitterBasis; }
float getMinParticleSize()      { return fountain.minParticleSize; }
float getMaxParticleSize()      { return fountain.maxParticleSize; }
float getDelta()                { return fountain.delta; }
float getVelocityLowerBound()   { return fountain.velLB; }
float getVelocityUpperBound()   { return fountain.velUB; }
