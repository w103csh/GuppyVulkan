/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_SCR_DEF 0
#define _DS_SMP_SCR_DEF 0

layout(set=_DS_UNI_SCR_DEF, binding=0) uniform ScreenSpaceDefault {
    vec4  weights0_3;               // gaussian normalized weights[0-3]
    vec4  weights4_7;               // gaussian normalized weights[4-7]
    vec2  weights8_9;               // gaussian normalized weights[8-9]
    float edgeThreshold;            //
    float luminanceThreshold;       //
    // 4 rem
} data;

layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;

// IN
layout(location=0) in vec2 fragTexCoord;
// OUT
layout(location=0) out vec4 outColor;

const vec3 LUMINANCE = vec3(0.2126, 0.7152, 0.0722);
float luminance(const in vec3 color) { return max(dot(LUMINANCE, color), 0.0); }

void main() {
    vec4 color = texture(sampRender, fragTexCoord);
    if( luminance(color.rgb) > data.luminanceThreshold ) {
        outColor = color;
    } else {
        outColor = vec4(0.0);
    }
}