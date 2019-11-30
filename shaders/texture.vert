/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// DECLARATIONS
void setProjectorTexCoord(const in vec4 pos);

// BINDINGS
layout(binding=0) uniform CameraDefaultPerspective {
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
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;
layout(location=5) in mat4 inModel;
// OUT
layout(location=0) out vec3 fragPosition; // (camera space)
layout(location=1) out vec3 fragNormal;   // (texture space)
layout(location=2) out vec2 fragTexCoord; // (texture space)
// layout(location=2) centroid out vec2 fragTexCoord; // (texture space)
layout(location=3) out mat3 TBN;

void main() {
    // Camera space transforms
    mat4 viewModel = camera.view * inModel;
    mat3 viewModel3 = mat3(viewModel);

    vec3 CS_normal = normalize(viewModel3 * inNormal);
    vec3 CS_tangent = normalize(viewModel3 * inTangent);
    vec3 CS_binormal = normalize(viewModel3 * inBinormal);

    vec4 pos = inModel * vec4(inPosition, 1.0);
    setProjectorTexCoord(pos);

    gl_Position = camera.viewProjection * pos;

    // Camera space to tangent space
    // TBN = inverse(mat3(CS_binormal, CS_tangent, CS_normal));
    TBN = transpose(mat3(CS_binormal, CS_tangent, CS_normal));

    fragPosition = (viewModel * vec4(inPosition, 1.0)).xyz;
    fragNormal = TBN * CS_normal;
    fragTexCoord = inTexCoord;
}
