/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_CAM_BSC_ONLY 0

layout(set=_DS_CAM_BSC_ONLY, binding=0) uniform BasicCamera {
    mat4 viewProj;
} cam;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;
layout(location=5) in mat4 inModel;

void main() {
    vec3 position = (inModel * vec4(inPosition, 1.0)).xyz;
    gl_Position = cam.viewProj * vec4(position, 1.0);
}