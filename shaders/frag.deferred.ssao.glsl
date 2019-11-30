/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

// UNIFORMS
#define _DS_UNI_DFR_SSAO 0
#define _DS_UNI_DFR_COMB 0
// SAMPLERS
#define _DS_SMP_DFR_SSAO_RAND 0
// ATTACHMENTS
#define _DS_SMP_DFR_POS 0
#define _DS_SMP_DFR_NORM 0

layout(set=_DS_UNI_DFR_SSAO, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

const uint KERNEL_SIZE = 64;
const uint KERNEL_ARRAY_SIZE = KERNEL_SIZE / 4;

layout(set=_DS_UNI_DFR_SSAO, binding=1) uniform SSAO {
    vec4 data[KERNEL_ARRAY_SIZE];
} kernel;

layout(set=_DS_SMP_DFR_SSAO_RAND, binding=0) uniform sampler2D sampSSAORand;

layout(input_attachment_index=2, set=_DS_SMP_DFR_POS, binding=0) uniform subpassInput posInput;
layout(input_attachment_index=3, set=_DS_SMP_DFR_NORM, binding=0) uniform subpassInput normInput;

// IN
layout(location=0) in vec2 inTexCoord;
// OUT
layout(location=0) out float outSsaoData;

const vec2 randScale = vec2( 800.0 / 4.0, 600.0 / 4.0 );

void main() {
    // Create the random tangent space matrix
    vec3 randDir = normalize(texture(sampSSAORand, inTexCoord.xy * randScale).xyz);
    vec3 n = normalize(subpassLoad(normInput).xyz);
    vec3 biTang = cross(n, randDir);
    if(length(biTang) < 0.0001)  // If n and randDir are parallel, n is in x-y plane
        biTang = cross(n, vec3(0.0, 0.0, 1));
    biTang = normalize(biTang);
    vec3 tang = cross(biTang, n);
    mat3 toCamSpace = mat3(tang, biTang, n);

    float occlusionSum = 0.0;
    vec3 camPos = subpassLoad(posInput).xyz;
    for (int i = 0; i < KERNEL_ARRAY_SIZE; i++) {
        for (int j = 0; j < 4; j++) {
            vec3 samplePos = camPos + Radius * (toCamSpace * kernel.data[i][j]);

            // Project point
            vec4 p = camera.projection * vec4(samplePos,1);
            p *= 1.0 / p.w;
            p.xyz = p.xyz * 0.5 + 0.5;

            // Access camera space z-coordinate at that point

            // DOING IT THIS WAY IN THE DEFERRED PASS WILL NOT WORK!!!
            // HERE YOU NEED TO BE ABLE TO ESSENTIALLY SAMPLE A POSITION
            // THAT MIGHT NOT BE AVAILABLE TO AN INPUT_ATTACHEMNT THAT
            // IS REGION/TILE DEPENDENT.

            float surfaceZ = texture(PositionTex, p.xy).z;
            float zDist = surfaceZ - camPos.z;
            
            // Count points that ARE occluded
            if (zDist >= 0.0 && zDist <= Radius && surfaceZ > samplePos.z) occlusionSum += 1.0;
        }
    }

    float occ = occlusionSum / KERNEL_SIZE;
    outSsaoData = 1.0 - occ;
    //FragColor = vec4(AoData, AoData, AoData, 1);
}