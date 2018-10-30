
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec4 getColor(vec3 ambientColor, vec3 diffuseColor, vec3 spectralColor, float opacity);

// IN
layout(location = 1) in vec4 fragColor;

// OUT
layout(location = 0) out vec4 outColor;

void main() {
    float opacity = fragColor[3];

    vec3 Ld, Ls;
    vec3 ambientColor = vec3(fragColor);
    vec3 diffuseColor = vec3(fragColor);
    vec3 spectralColor = vec3(0.3, 0.3, 0.3); // Should this smaller?

    outColor = getColor(ambientColor, diffuseColor, spectralColor, opacity);
}