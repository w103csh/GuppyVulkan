
#version 450

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 outNormal;
layout(location=1) out vec4 outColor;

void main() {
    gl_Position = inModel * vec4(inPosition, 1.0);
    outNormal = mat3(inModel) * inNormal;
    outColor = inColor;
}