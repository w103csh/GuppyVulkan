/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// BINDINGS
layout(binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;
layout(location=5) in mat4 inModel;
// OUT
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec2 outTexCoord;
layout(location=3) out vec3 outTangent;     // (world space)
layout(location=4) out vec3 outBinormal;    // (world space)

void main() {
    mat3 model3 = mat3(inModel);
    outNormal = normalize(model3 * inNormal);
    outTangent = normalize(model3 * inTangent);
    outBinormal = normalize(model3 * inBinormal);
    outTexCoord = inTexCoord;
    gl_Position = inModel * vec4(inPosition, 1.0);
}
