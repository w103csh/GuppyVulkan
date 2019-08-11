    
#version 450

#define _DS_UNI_DFR_MRT 0
#define _DS_SMP_DEF 0

layout(set=_DS_UNI_DFR_MRT, binding=1) uniform MaterialDefault {
    vec3 color;             // Diffuse color for dielectrics, f0 for metallic
    float opacity;          // Overall opacity
    // 16
    uint flags;             // Flags (general/material)
    uint texFlags;          // Flags (texture)
    float xRepeat;          // Texture xRepeat
    float yRepeat;          // Texture yRepeat
    // 16
    vec3 Ka;                // Ambient reflectivity
    float shininess;        // Specular shininess factor
    // 16
    vec3 Ks;                // Specular reflectivity
    float eta;              // Index of refraction
    // 16
    float reflectionFactor; // Percentage of reflected light
} material;

layout(set=_DS_SMP_DEF, binding=0) uniform sampler2DArray sampColor;
#ifdef SOMETHING
layout (binding = 2) uniform sampler2D sampNormalMap;
#endif


// IN
layout(location=0) in vec3 inPosition;    // (world space)
layout(location=1) in vec3 inNormal;      // (world space)
layout(location=2) in vec3 inTangent;     // (world space)
layout(location=3) in vec3 inBinormal;    // (world space)
layout(location=4) in vec2 inTexCoord;

// OUT
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outColor;

void main() 
{
	outPosition = vec4(inPosition, 1.0);
    vec2 texCoord = vec2((inTexCoord.x * material.xRepeat), (inTexCoord.y * material.yRepeat));

    vec3 N = normalize(inNormal);
#ifdef SOMETHING
    vec3 T = normalize(inTangent);
    vec3 B = normalize(inBinormal);
    mat3 TBN = mat3(T, B, N);

    vec3 tnorm = TBN * normalize(texture(samplerNormalMap, texCoord).xyz * 2.0 - vec3(1.0));
    outNormal = vec4(tnorm, 1.0);
#else
    outNormal = vec4(N, 0.0);
#endif

	outColor = texture(sampColor, vec3(texCoord, 0));
}