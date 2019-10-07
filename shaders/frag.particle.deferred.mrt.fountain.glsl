    
#version 450

#define _DS_UNI_PRTCL_FNTN 0
#define _DS_SMP_DEF 0

layout(set=_DS_UNI_PRTCL_FNTN, binding=1) uniform ParticleFountain {
    // Material::Base::DATA
    vec3 color;             // Diffuse color for dielectrics, f0 for metallic
    float opacity;          // Overall opacity
    // 16
    uint flags;             // Flags (general/material)
    uint texFlags;          // Flags (texture)
    float xRepeat;          // Texture xRepeat
    float yRepeat;          // Texture yRepeat
} uniFountain;

layout(set=_DS_SMP_DEF, binding=0) uniform sampler2DArray sampParticle;

// IN
layout(location=0) in vec3 inPosition;        // camera space
layout(location=1) in float inTransparency;
layout(location=2) in vec2 inTexCoord;

// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;

// FLAGS
const uint PER_MATERIAL_COLOR       = 0x00000001u;
const uint PER_VERTEX_COLOR         = 0x00000002u;
const uint PER_TEXTURE_COLOR        = 0x00000004u;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n;      // normal
float opacity, height;

void main() {
    
    if ((uniFountain.flags & PER_TEXTURE_COLOR) > 0) {

        vec2 texCoord = vec2(
            (inTexCoord.x * uniFountain.xRepeat),
            (inTexCoord.y * uniFountain.yRepeat)
        );

        vec4 samp = texture(sampParticle, vec3(texCoord, 0));

        Kd = samp.xyz;
        Ks = vec3(0.5);
        Ka = Kd;
        opacity = samp[3];

    } else {

        Kd = uniFountain.color;
        Ks = vec3(0.5);
        Ka = Kd;
        opacity = uniFountain.opacity;

    }
    
    outDiffuse = vec4(Kd, opacity * inTransparency);
    if (outDiffuse[3] < 0.15)
        discard;
    outPosition = vec4(inPosition, 1.0);
    outNormal = vec4(0);
    outAmbient = vec4(Ka, -2.0);
    outSpecular = vec4(Ks, 0);
}