/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// DECLARATIONS
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 inPosition;  // not used
layout(location=1) in vec3 inNormal;    // not used
layout(location=2) in vec4 inColor;
// OUT
layout(location=0) out vec4 outColor;

void main() {
    outColor = gammaCorrect(inColor.rgb, inColor.a);
}