
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_UNI_SCR_DEF 0

layout(set=_DS_UNI_SCR_DEF, binding=0) uniform ScreenSpaceDefault {
    float edgeThreshold;          //
    // 12 rem
} screenSpaceData;

// IN
layout(location=0) in vec3 inPosition;

void main() {
    gl_Position = vec4(inPosition, 1.0);
}