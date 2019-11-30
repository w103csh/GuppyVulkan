/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
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

layout(set=_DS_HFF, binding=4) buffer Normals {
    vec4 normals[];
};

// IN
layout(local_size_x=4, local_size_y=4) in;

void main() {
  uint idx = gl_GlobalInvocationID.x + (gl_GlobalInvocationID.y * (sim.mMinus1 + 1));

  vec3 p = vec3(0, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.xy, sim.write)).r, 0);                                               // p (0, 0, 0)
  vec3 n = vec3(0);
  vec3 a, b, c;

  if (gl_GlobalInvocationID.y < sim.nMinus1 - 1) {
    c = vec3(0, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y + 1, sim.write)).r, -sim.h) - p;             // c (0, 0, -1)
    if (gl_GlobalInvocationID.x < sim.mMinus1 - 1) {
      a = vec3(-sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x + 1, gl_GlobalInvocationID.y, sim.write)).r, 0) - p;           // a (-1, 0, 0)
      b = vec3(-sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x + 1, gl_GlobalInvocationID.y + 1, sim.write)).r, -sim.h) - p;  // b (-1, 0, -1)
      // p a
      //   b
      n += cross(b, a);
      // p
      // c b
      n += cross(c, b);
    }
    if (gl_GlobalInvocationID.x > 0) {
      a = c;                                                                                                                                 // a (0, 0, -1)
      b = vec3(sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x - 1, gl_GlobalInvocationID.y + 1, sim.write)).r, -sim.h) - p;  // b (1, 0, -1)
      c = vec3(sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x - 1, gl_GlobalInvocationID.y, sim.write)).r, 0) - p;           // c (1, 0, 0)
      //   P
      // b a
      n += cross(b, a);
      // c p
      // b
      n += cross(c, b);
    }
  }

  if (gl_GlobalInvocationID.y > 0) {
    c = vec3(0, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y - 1, sim.write)).r, sim.h) - p;             // c (0, 0, 1)
    if (gl_GlobalInvocationID.x > 0) {
      a = vec3(sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x - 1, gl_GlobalInvocationID.y, sim.write)).r, 0) - p;           // a (1, 0, 0)
      b = vec3(sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x - 1, gl_GlobalInvocationID.y - 1, sim.write)).r, sim.h) - p;   // b (1, 0, 1)
      // b
      // a p
      n += cross(b, a);
      // b c
      //   p
      n += cross(c, b);
    }
    if (gl_GlobalInvocationID.x < sim.mMinus1 - 1) {
      a = c;
      b = vec3(-sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x + 1, gl_GlobalInvocationID.y - 1, sim.write)).r, sim.h) - p;   // b (-1, 0, 1)
      c = vec3(-sim.h, imageLoad(imgHeightField, ivec3(gl_GlobalInvocationID.x + 1, gl_GlobalInvocationID.y, sim.write)).r, 0) - p;           // c (-1, 0, 0)
      // a b
      // p
      n += cross(b, a);
      //   b
      // p c
      n += cross(c, b);
    }
  }

  normals[idx] = vec4(normalize(n), 0.0);
}

