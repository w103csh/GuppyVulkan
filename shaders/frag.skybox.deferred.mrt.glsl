/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
     
#version 450

#define _DS_SMP_DEF 0

layout(set=_DS_SMP_DEF, binding=0) uniform samplerCube sampCube;

// IN
layout(location=0) in vec3 inPosition;    // (world space)
// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;
layout(location=5) out uint outFlags;

void main() {
    outPosition = vec4(inPosition, 1.0);
    outDiffuse = vec4(texture(sampCube, inPosition).rgb, 1.0);
    outFlags = 0x01u;
}