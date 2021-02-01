/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_UNI_DEF 0

// DECLARATIONS
void setProjectorTexCoord(const in vec4 pos);

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// // PUSH CONSTANTS
// layout(push_constant) uniform PushBlock {
//     mat4 model;
// } pushConstantsBlock;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;
// layout(location=3) out int outVertexIndex;

void main() {
    // This obviously can be much more efficient. (Can it??)
    mat4 viewModel = camera.view * inModel;

    outPosition = (viewModel * vec4(inPosition, 1.0)).xyz;
    outNormal = normalize(mat3(viewModel) * inNormal);
    outColor = inColor;
    
    vec4 pos = inModel * vec4(inPosition, 1.0);
    setProjectorTexCoord(pos);

    gl_Position = camera.viewProjection * pos;
    // outVertexIndex = gl_VertexIndex;
}
