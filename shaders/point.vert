/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_UNI_DEF 0

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 outPosition;    // (camera space)
layout(location=1) out vec3 outNormal;      // (camera space)
layout(location=2) out vec4 outColor;

void main() { inModel;
    mat4 viewModel = camera.view * inModel;
    outPosition = (viewModel * vec4(inPosition, 1.0)).xyz;
    outNormal = normalize(mat3(viewModel) * inNormal);
    outColor = inColor;
    gl_Position = camera.viewProjection * vec4(inPosition, 1.0);
    gl_PointSize = inNormal.x;
}
