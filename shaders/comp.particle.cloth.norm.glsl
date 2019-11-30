/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_PRTCL_CLTH_NRM 0

// STORAGE BUFFERS
layout(set=_DS_PRTCL_CLTH_NRM, binding=0) buffer readonly Positions {
    vec4 positions[];
};
layout(set=_DS_PRTCL_CLTH_NRM, binding=1) buffer Normals {
    vec4 normals[];
};

// IN
layout(local_size_x = 10, local_size_y = 10) in;

void main() {
  uvec3 nParticles = gl_NumWorkGroups * gl_WorkGroupSize;
  uint idx = gl_GlobalInvocationID.y * nParticles.x + gl_GlobalInvocationID.x;

  vec3 p = vec3(positions[idx]);
  vec3 n = vec3(0);
  vec3 a, b, c;

  if (gl_GlobalInvocationID.y > 0) {
    c = positions[idx - nParticles.x].xyz - p;
    if (gl_GlobalInvocationID.x < nParticles.x - 1) {
      a = positions[idx + 1].xyz - p;
      b = positions[idx - nParticles.x + 1].xyz - p;
      n += cross(a, b);
      n += cross(b, c);
    }
    if (gl_GlobalInvocationID.x > 0) {
      a = c;
      b = positions[idx - nParticles.x - 1].xyz - p;
      c = positions[idx - 1].xyz - p;
      n += cross(a, b);
      n += cross(b, c);
    }
  }

  if (gl_GlobalInvocationID.y < nParticles.y - 1) {
    c = positions[idx + nParticles.x].xyz - p;
    if (gl_GlobalInvocationID.x > 0) {
      a = positions[idx - 1].xyz - p;
      b = positions[idx + nParticles.x - 1].xyz - p;
      n += cross(a, b);
      n += cross(b, c);
    }
    if (gl_GlobalInvocationID.x < nParticles.x - 1) {
      a = c; 
      b = positions[idx + nParticles.x + 1].xyz - p;
      c = positions[idx + 1].xyz - p;
      n += cross(a, b);
      n += cross(b, c);
    }
  }

  normals[idx] = vec4(normalize(n), 0.0);
}

