
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=1, binding=0) uniform samplerCube sampCube;
// INin
layout(location=0) in vec3 fragPosition;    // (camera space)
layout(location=1) in vec3 fragNormal;      // (camera space) NOT USED
layout(location=2) in vec4 fragColor;       // (camera space) NOT USED
// OUT
layout(location=0) out vec4 outColor;

void main() {
    // outColor = vec4(1.0, 0.0, 0.0, 1.0);
    vec3 color = texture(sampCube, normalize(fragPosition)).rgb;
    // color = pow(color, vec3(1.0/2.2));
    outColor = vec4(color,1);
}