/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_OCEAN 0
#define _DS_CDLOD 0

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_OCEAN, binding=2) uniform sampler2DArray sampVertInput;
layout(set=_DS_CDLOD, binding=0) uniform CdlodQuadTree {
    vec4 terrainScale;
    vec4 terrainOffset;
    vec4 heightmapTextureInfo;
    vec2 quadWorldMax;  // .xy max used to clamp triangles outside of world range
    vec2 samplerWorldToTextureScale;
} qTree;

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    vec4 data0;  // gridDim:         .x (dimension), .y (dimension/2), .z (2/dimension)
                 //                  .w (LODLevel)
    vec4 data1;  // morph constants: .x (start), .y (1/(end-start)), .z (end/(end-start))
                 //                  .w ((aabb.minZ+aabb.maxZ)/2)
    vec4 data2;  // quadOffset:      .x (aabb.minX), .y (aabb.minY)
                 // quadScale:       .z (aabb.sizeX), .w (aabb.sizeY)
    vec4 data3;  // dbg camera:      .x (wpos.x), .y (wpos.y), .z (wpos.z)
} pc;

// IN
layout(location=0) in vec2 inPosition;

// OUT
layout(location=0) out vec3 outPosition; // (world space)
layout(location=1) out vec3 outNormal;   // (world space)
layout(location=2) out vec4 outColor;

const int LAYER_POSITION  = 0;
const int LAYER_NORMAL    = 1;

// morphs vertex xy from from high to low detailed mesh position
vec2 morphVertex(vec2 inPos, vec2 vertex, float morphLerpK) {
    vec2 fracPart = (fract(inPos * vec2(pc.data0.y, pc.data0.y)) *
                     vec2(pc.data0.z, pc.data0.z)) *
                    pc.data2.zw;
    return vertex.xy - fracPart * morphLerpK;
}

void main() {
    // getBaseVertexPos
    vec4 vertex = vec4(((inPosition * pc.data2.zw) + pc.data2.xy), 0.0, 1.0);
    vertex.xy = min(vertex.xy, qTree.quadWorldMax); // !!!

    // calcGlobalUV
    vec2 uv = vertex.xy * qTree.heightmapTextureInfo.zw;

    // sampleHeightmap
    vec4 posData = texture(sampVertInput, vec3(uv, LAYER_POSITION));  // .xy: displacement
                                                                      // .z:  height
    vertex.z = posData.z;
    // vertex.xy += posData.xy; // This causes seems.

    // swizzle to convert to z-up left-handed coordinate system
    vec3 camPos = (pc.data3.w == 1.0) ? pc.data3.xzy : camera.worldPosition.xzy;
    float eyeDist = distance(vertex, vec4(camPos, 1.0));

    float morphLerpK = 1.0f - clamp(pc.data1.z - eyeDist * pc.data1.y, 0.0, 1.0 );
    vertex.xy = morphVertex(inPosition, vertex.xy, morphLerpK);

    // calcGlobalUV
    uv = vertex.xy * qTree.heightmapTextureInfo.zw;

    // sampleHeightmap
    posData = texture(sampVertInput, vec3(uv, LAYER_POSITION));  // .xy: displacement
                                                                 // .z:  height
    vertex.z = posData.z;
    vertex.xy += posData.xy;

    // Position
    outPosition = vertex.xzy; // Swizzle z/y up.
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);
    // Normal
    outNormal = texture(sampVertInput, vec3(uv, LAYER_NORMAL)).xyz;
}
