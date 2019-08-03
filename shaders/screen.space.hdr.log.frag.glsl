
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_SMP_SCR_DEF 0

layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;

// IN
layout(location=0) in vec2 fragTexCoord;
// OUT
layout(location=0) out float outLog;

const vec3 LUMINANCE = vec3(0.2126, 0.7152, 0.0722);
float luminance(const in vec3 color) { 
    float lum = dot(LUMINANCE, color);
    lum = max(lum, 0.0);
    return lum;
}

void main() {
    vec3 color = texelFetch(sampRender, ivec2(gl_FragCoord.xy), 0).rgb;
    float lum = luminance(color);
    outLog = log(lum + 0.00001f);
}