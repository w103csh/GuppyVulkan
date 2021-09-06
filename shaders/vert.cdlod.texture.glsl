/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_UNI_DEF 0

// DECLARATIONS
void ProcessCDLODVertex(
    vec4 inPos,
    out vec4 outUnmorphedWorldPos,
    out vec4 outWorldPos,
    out vec2 outGlobalUV,
    out vec2 outDetailUV,
    out vec2 outMorphK,
    out float outEyeDist
);

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec2 inPosition;
// OUT
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec2 outTexCoord;
layout(location=3) out vec3 outTangent;     // (world space)
layout(location=4) out vec3 outBinormal;    // (world space)
layout(location=5) out flat uint outFlags;

void main() {
    vec4 inPos = vec4(inPosition, 0.0, 1.0);
    vec4 unmorphedWorldPos;
    vec4 worldPos;
    vec2 globalUV;
    vec2 detailUV;
    vec2 morphK;
    float eyeDist;
    ProcessCDLODVertex( inPos, unmorphedWorldPos, worldPos, globalUV, detailUV, morphK, eyeDist );

    // Position
    outPosition = worldPos.xzy; // swizzle to convert from z-up left-handed coordinate system
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // Normal
    outNormal = vec3(0.0, 1.0, 0.0);
    // Texture coordinate
    // outTexCoord = globalUV;
    outTexCoord = detailUV;
    // Flags
    outFlags = 0x00u;
}
