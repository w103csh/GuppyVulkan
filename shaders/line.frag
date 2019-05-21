
#version 450
#extension GL_ARB_separate_shader_objects : enable

// IN
layout(location = 0) in vec3 fragPos;       // not used
layout(location = 1) in vec3 fragNormal;    // not used
layout(location = 2) in vec4 fragColor;
// OUT
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}