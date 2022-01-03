/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

layout(location=0) in vec3 inPosition;
layout(location=3) in mat4 inModel;

void main() {
    gl_Position = inModel * vec4(inPosition, 1.0);
}