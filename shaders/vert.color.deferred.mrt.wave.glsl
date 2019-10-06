
#version 450

#define _DS_UNI_DEF 0
#define _DS_UNI_PRTCL_WV 0

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=_DS_UNI_PRTCL_WV, binding=0) uniform ParticleWave {
    float time;
    float freq;
    float velocity;
    float amp;
} uniWave;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;

// OUT
layout(location=0) out vec3 outPosition;    // (camera space)
layout(location=1) out vec3 outNormal;      // (camera space)
layout(location=2) out vec4 outColor;

const float PI = 3.1415926535897932384626433832795;
const float TAU = 2 * PI;

void main() {
    vec4 pos = inModel * vec4(inPosition, 1.0);

    // Wave
    float u = (TAU / uniWave.freq) * (pos.x - (uniWave.velocity * uniWave.time));
    // float u = (TAU / 2.5) * (pos.x - (1.0 * uniWave.time));
    
    pos.y += uniWave.amp * sin(u);
    vec3 n = vec3(0.0);
    n.xy = vec2(cos(u), 1.0);
    
    gl_Position = camera.viewProjection * pos;

    mat4 mViewModel = camera.view * inModel;
    mat3 mNormal = transpose(inverse(mat3(mViewModel)));

    outPosition = (mViewModel * vec4(inPosition, 1.0)).xyz;
    outNormal = mNormal * normalize(n);
    outColor = inColor;
}