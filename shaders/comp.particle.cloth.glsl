
#version 450

#define _DS_PRTCL_CLTH 0

// UNIFORMS
layout(set=_DS_PRTCL_CLTH, binding=0) uniform ParticleAttractor {
    /**
     * data[0-2]: gravity
     * data[3]: springK
     */
    vec4 data;
    float mass;
    float inverseMass;
    float restLengthHoriz;
    float restLengthVert;
    float restLengthDiag;
    float dampingConst;
    float delta;
} uniCloth;

// const float DeltaT = 0.000005;

// STORAGE BUFFERS
layout(set=_DS_PRTCL_CLTH, binding=1) buffer Position0 {
    vec4 positionsIn[];
};
layout(set=_DS_PRTCL_CLTH, binding=2) buffer Position1 {
    vec4 positionsOut[];
};
layout(set=_DS_PRTCL_CLTH, binding=3) buffer Velocity0 {
    vec4 velocitiesIn[];
};
layout(set=_DS_PRTCL_CLTH, binding=4) buffer Velocity1 {
    vec4 velocitiesOut[];
};

// IN
layout(local_size_x=10, local_size_y=10) in;

void main() {
    uvec3 nParticles = gl_NumWorkGroups * gl_WorkGroupSize;
    uint idx = gl_GlobalInvocationID.y * nParticles.x + gl_GlobalInvocationID.x;

    vec3 p = vec3(positionsIn[idx]);

    // Pin a few of the top verts
    if (gl_GlobalInvocationID.y == 0 
        && (gl_GlobalInvocationID.x == 0 || 
        gl_GlobalInvocationID.x == nParticles.x / 4 ||
        gl_GlobalInvocationID.x == nParticles.x * 2 / 4 ||
        gl_GlobalInvocationID.x == nParticles.x * 3 / 4 ||
        gl_GlobalInvocationID.x == nParticles.x - 1)
        ) {
        positionsOut[idx] = vec4(p, 1.0);
        velocitiesOut[idx] = vec4(0.0);
        return;
    }

    vec3 v = vec3(velocitiesIn[idx]), r;

    // Start with gravitational acceleration and add the spring
    // forces from each neighbor
    vec3 force = uniCloth.data.xyz * uniCloth.mass;

    int thing = 0;

    // Cardinals
    // Above
    if (gl_GlobalInvocationID.y > 0) {
        r = positionsIn[idx - nParticles.x].xyz - p;
        thing += int(idx - nParticles.x);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthVert);
    } 
    // Below
    if (gl_GlobalInvocationID.y < nParticles.y - 1) {
        r = positionsIn[idx + nParticles.x].xyz - p;
        thing += int(idx + nParticles.x);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthVert);
    } 
    // Left
    if (gl_GlobalInvocationID.x > 0) {
        r = positionsIn[idx - 1].xyz - p;
        thing += int(idx - 1);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthHoriz);
    } 
    // Right
    if (gl_GlobalInvocationID.x < nParticles.x - 1) {
        r = positionsIn[idx + 1].xyz - p;
        thing += int(idx + 1);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthHoriz);
    }

    // Diagonals
    // Upper-left
    if (gl_GlobalInvocationID.x > 0 && gl_GlobalInvocationID.y > 0) {
        r = positionsIn[idx - nParticles.x - 1].xyz - p;
        thing += int(idx - nParticles.x - 1);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthDiag);
    }
    // Upper-right
    if (gl_GlobalInvocationID.x < nParticles.x - 1 && gl_GlobalInvocationID.y > 0) {
        r = positionsIn[idx - nParticles.x + 1].xyz - p;
        thing += int(idx - nParticles.x + 1);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthDiag);
    }
    // lower-left
    if (gl_GlobalInvocationID.x > 0 && gl_GlobalInvocationID.y < nParticles.y - 1) {
        r = positionsIn[idx + nParticles.x - 1].xyz - p;
        thing += int(idx + nParticles.x - 1);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthDiag);
    }
    // lower-right
    if (gl_GlobalInvocationID.x < nParticles.x - 1 && gl_GlobalInvocationID.y < nParticles.y - 1) {
        r = positionsIn[idx + nParticles.x + 1].xyz - p;
        thing += int(idx + nParticles.x + 1);
        force += normalize(r) * uniCloth.data.w * (length(r) - uniCloth.restLengthDiag);
    }

    force += -uniCloth.dampingConst * v;

    // Apply simple Euler integrator for Newton's Law of Motion
    vec3 a = force * uniCloth.inverseMass;
    positionsOut[idx] = vec4(p +
        v * uniCloth.delta +
        0.5 * a * uniCloth.delta * uniCloth.delta,
        1.0
    );
    velocitiesOut[idx] = vec4(v + a * uniCloth.delta, 0.0);

    // positionsOut[idx] = vec4(p +
    //     v * DeltaT +
    //     0.5 * a * DeltaT * DeltaT,
    //     1.0
    // );
    // velocitiesOut[idx] = vec4(v + a * DeltaT, 0.0);
}