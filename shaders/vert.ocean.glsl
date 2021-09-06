/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_OCEAN 0

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_OCEAN, binding=2) uniform sampler2DArray sampVertInput;

// IN
layout(location=0) in vec2 inPosition;
layout(location=1) in vec4 data0;      // quadOffset: .x.y
                                       // quadScale:  .z.w
layout(location=2) in vec4 data1;      // uvOffset:   .x.y
                                       // uvScale:    .z.w

// OUT
layout(location=0) out vec3 outPosition; // (world space)
layout(location=1) out vec3 outNormal;   // (world space)
layout(location=2) out vec4 outColor;

const int LAYER_POSITION  = 0;
const int LAYER_NORMAL    = 1;

#define QUAD_OFFSET_V2 data0.xy
#define QUAD_SCALE_V2  data0.zw
#define UV_OFFSET_V2   data1.xy
#define UV_SCALE_V2    data1.zw

void main() {
    const vec2 texCoord = (inPosition * UV_SCALE_V2) + (UV_OFFSET_V2);

    // Position
    vec4 posData = texture(sampVertInput, vec3(texCoord, LAYER_POSITION));  // .xy: displacement
                                                                            // .z:  height
    vec2 xz = (inPosition * QUAD_SCALE_V2) + (QUAD_OFFSET_V2 + posData.xy);
    outPosition = vec3(xz.x, posData.z, xz.y);
    gl_Position = camera.viewProjection * vec4(outPosition, 1.0);

    // Normal
    outNormal = texture(sampVertInput, vec3(texCoord, LAYER_NORMAL)).xyz;
}
