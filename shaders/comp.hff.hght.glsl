
#version 450

#define _DS_HFF 0

layout(set=_DS_HFF, binding=2) uniform Simulation {
    float c2;        // wave speed
    float h;         // distance between heights
    float h2;        // h squared
    float dt;        // time delta
    float maxSlope;  // clamped sloped to prevent numerical explosion
    int read, write;
    int mMinus1, nMinus1;
} sim;
layout(set=_DS_HFF, binding=3, r32f) uniform image3D imgHeightField;

const int VELOCITY_LAYER = 2;
const int DISPLACE_LAYER = 3;

// IN
layout(local_size_x=4, local_size_y=4) in;

void main() {
    /**
     *  forall i,j
     *      f = c2*(u[i+1,j]+u[i-1,j]+u[i,j+1]+u[i,j-1]
     *                  – 4u[i,j])/h2
     *      v[i,j] = v[i,j] + f*∆t
     *      unew[i,j] = u[i,j] + v[i,j]*∆t
     *  endfor
     *  forall i,j: u[i,j] = unew[i,j]
     */

    float u = imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.xy, sim.read)).r;

    float offset =
        // u[i+1,j] +
        imageLoad(imgHeightField, ivec3(min(gl_GlobalInvocationID.x + 1, sim.mMinus1), gl_GlobalInvocationID.y, sim.read)).r +
        // u[i-1,j] +
        imageLoad(imgHeightField, ivec3(max(int(gl_GlobalInvocationID.x) - 1, 0), gl_GlobalInvocationID.y, sim.read)).r +
        // u[i,j+1] +
        imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x, min(gl_GlobalInvocationID.y + 1, sim.nMinus1), sim.read)).r +
        // u[i,j-1] –
        imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x, max(int(gl_GlobalInvocationID.y) - 1, 0), sim.read)).r -
        // 4 * u[i,j]
        4 * u;

    // // Clamping
    // float maxOffset = sim.maxSlope * sim.h;
    
    // if (offset > maxOffset) {

    //     offset -= maxOffset;
    //     imageStore(imgHeightField, ivec3(gl_GlobalInvocationID.xy, 5), vec4((offset / sim.dt), 0, 0, 0));
    //     imageStore(imgHeightField, ivec3(gl_GlobalInvocationID.xy, 4), vec4(u + offset, 0, 0, 0));

    // } else if (offset < -maxOffset) {

    //     offset += maxOffset;
    //     imageStore(imgHeightField, ivec3(gl_GlobalInvocationID.xy, 5), vec4((offset / sim.dt), 0, 0, 0));
    //     imageStore(imgHeightField, ivec3(gl_GlobalInvocationID.xy, 4), vec4(u + offset,0,0,0));

    // } 
    // else {

        float f = sim.c2 * offset / sim.h2;

        // v[i,j] + f*∆t
        float v = imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.xy, VELOCITY_LAYER)).r + f * sim.dt;
        imageStore(imgHeightField, ivec3(gl_GlobalInvocationID.xy, VELOCITY_LAYER), vec4(v, 0, 0, 0));

        imageStore(imgHeightField, ivec3(gl_GlobalInvocationID.xy, sim.write), vec4(
            // u[i,j] + v[i,j]*∆t
            u + v * sim.dt
        , 0, 0, 0));

    // }

}