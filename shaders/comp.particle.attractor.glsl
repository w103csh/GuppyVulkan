/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_PRTCL_ATTR 0

// UNIFORMS
layout(set=_DS_PRTCL_ATTR, binding=0) uniform ParticleAttractor {
    /**
     * vec4[0-2]: position
     * vec4[3]: gravity
     */
    vec4 data[2];
    float inverseMass;
    float maxDistance;
    float delta;
} uniAttr;

// STORAGE BUFFERS
layout(set=_DS_PRTCL_ATTR, binding=1, std140) buffer Position {
    vec4 positions[];
};
layout(set=_DS_PRTCL_ATTR, binding=2, std140) buffer Velocity {
    vec4 velocities[];
};

// LOCAL SIZE
layout(local_size_x=1000) in;

void main() {
    uint index = gl_GlobalInvocationID.x;

    vec3 p = positions[index].xyz;

    // Force from attractor #0
    vec3 d = uniAttr.data[0].xyz - p;
    float dist = length(d);
    vec3 force = (uniAttr.data[0].w / dist) * normalize(d);

    // Force from attractor #1
    d = uniAttr.data[1].xyz - p;
    dist = length(d);
    force += (uniAttr.data[1].w / dist) * normalize(d);

    // Reset particles that get too far from the attractors
    if(dist > uniAttr.maxDistance) {
        positions[index] = vec4(0,0,0,1);
    } else {
        // Apply simple Euler integrator
        vec3 a = force * uniAttr.inverseMass;
        positions[index] = vec4(p +
            velocities[index].xyz * uniAttr.delta + 0.5 * a * uniAttr.delta * uniAttr.delta, 1.0);
        velocities[index] = vec4(velocities[index].xyz + a * uniAttr.delta, 0.0);
    }
}