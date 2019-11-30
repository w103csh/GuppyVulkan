/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// DECLARATIONS
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location=0) in vec3 fragPos;       // not used
layout(location=1) in vec3 fragNormal;    // not used
layout(location=2) in vec4 fragColor;
// OUT
layout(location=0) out vec4 outColor;

void main() {
    outColor = gammaCorrect(fragColor.rgb, fragColor.a);
}