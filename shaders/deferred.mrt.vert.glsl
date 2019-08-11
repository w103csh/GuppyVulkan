
#version 450

#define _DS_UNI_DFR_MRT 0

// BINDINGS
layout(set=_DS_UNI_DFR_MRT, binding=0) uniform CameraDefaultPerspective {
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

// OUT
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec3 outTangent;     // (world space)
layout(location=3) out vec3 outBinormal;    // (world space)
layout(location=4) out vec2 outTexCoord;

void main() {
    vec4 pos = inModel * vec4(inPosition, 1.0);
    // Camera space position
    gl_Position = camera.viewProjection * pos;
    outPosition = pos.xyz;
    // Normal matrix
    mat3 mNormal = transpose(inverse(mat3(inModel)));
    outNormal = mNormal * normalize(inNormal);
    outTangent = mNormal * normalize(inTangent);
    outBinormal = mNormal * normalize(inBinormal);

    outTexCoord = inTexCoord;
}