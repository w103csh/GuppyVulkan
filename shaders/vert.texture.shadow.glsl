
#version 450

#define _DS_UNI_SHDW 0

// BINDINGS
layout(set=_DS_UNI_SHDW, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBinormal;
layout(location=5) in mat4 inModel;

void main() {
    vec4 pos = inModel * vec4(inPosition, 1.0);
    gl_Position = camera.viewProjection * pos;
}
