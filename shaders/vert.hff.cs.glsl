
#version 450

#define _DS_HFF_DEF 0

// BINDINGS
layout(set=_DS_HFF_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_HFF_DEF, binding=2) uniform Simulation {
    float c2;        // wave speed
    float h;         // distance between heights
    float h2;        // h squared
    float dt;        // time delta
    float maxSlope;  // clamped sloped to prevent numerical explosion
    int read, write;
    int mMinus1, nMinus1;
} sim;
layout(set=_DS_HFF_DEF, binding=3, r32f) uniform readonly image3D imgHeightField;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in ivec2 inImageOffset;
layout(location=2) in vec4 inNormal;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;        // (camera space)
layout(location=1) out vec3 outNormal;          // (camera space)
layout(location=2) out vec4 outColor;
layout(location=3) out flat uint outFlags;

void main() {
    mat4 mViewModel = camera.view * inModel;
    // mat3 mNormal = transpose(inverse(mat3(mViewModel)));

    outPosition = inPosition;

    outPosition.y += imageLoad(imgHeightField, ivec3(inImageOffset, sim.write)).r;

    outPosition = (mViewModel * vec4(outPosition, 1.0)).xyz;
    // outNormal = normalize(mNormal * inNormal.xyz);
    outNormal = normalize(mat3(mViewModel) * inNormal.xyz);
    outFlags = 0x0u;
    // if (gl_VertexIndex == 0)
    //     outColor = vec4(0,1,0,1);
    // else
    //     outColor = vec4(1,0,0,1);

    gl_Position = camera.projection * vec4(outPosition, 1.0);
}