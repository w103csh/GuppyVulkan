/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

const float PI = 3.14159265358979323846;

vec2 complexMultiply(const in vec2 a, const in vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

float complexMagnitude(const in vec2 a) {
    return sqrt(a.x * a.x + a.y * a.y);
}