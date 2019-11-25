
#version 450

#define _DS_UNI_DEFCAM_DEFMAT_MX4 0

// DECLARATIONS
vec3  getAcceleration();
float getLifespan();
vec3  getEmitterPosition();
float getDelta();
float getMinParticleSize();

// BINDINGS
layout(set=_DS_UNI_DEFCAM_DEFMAT_MX4, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_UNI_DEFCAM_DEFMAT_MX4, binding=2) uniform Matrix4 {
    mat4 model;
} uniMat4;


// IN
layout(location=0) in vec3 inVelocity;
layout(location=1) in float inTimeOfBirth;

// OUT
layout(location=0) out vec3 outPosition;        // (camera space)
layout(location=1) out vec3 outNormal;          // (camera space)
layout(location=3) out vec2 outTexCoord;
layout(location=4) out float outTransparency;
layout(location=5) out flat uint outFlags;

// Offsets to the position in camera coordinates for each vertex of the particle's quad
const vec3 offsets[] = vec3[](vec3(-0.5, -0.5, 0), vec3(0.5, -0.5, 0), vec3( 0.5, 0.5, 0),
                              vec3(-0.5, -0.5, 0), vec3(0.5,  0.5, 0), vec3(-0.5, 0.5, 0));
// Texture coordinates for each vertex of the particle's quad
const vec2 texCoords[] = vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(0, 1));

void main() {
    mat4 mViewModel = camera.view * uniMat4.model;

    float t = getDelta() - inTimeOfBirth;
    if (t >= 0 && t < getLifespan()) {
        vec3 pos = getEmitterPosition() +
            (inVelocity * t) +
            (getAcceleration() * t * t);
        outPosition = (mViewModel * vec4(pos, 1.0)).xyz +
            (offsets[gl_VertexIndex] * getMinParticleSize());
        outTransparency = mix(1.0, 0.0, t / getLifespan());
    } else {
        // Particle doesn't "exist", draw fully transparent
        outPosition = vec3(0.0);
        outTransparency = 0.0;
    }

    outNormal = vec3(0.0, 0.0, 1.0);
    outTexCoord = texCoords[gl_VertexIndex];
    outFlags = 0x0u;

    gl_Position = camera.projection * vec4(outPosition, 1.0);
}