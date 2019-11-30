/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;
// OUT
layout(location=0) out vec2 outTexCoord;

void main() {
    outTexCoord = inTexCoord;
    gl_Position.x =  inPosition.x;
    gl_Position.y = -inPosition.y; // Vulkan y is inverted
    gl_Position.z =  inPosition.z;
    gl_Position.w =  1.0;
}