/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_STR_SCR_CMP_PSTPRC 0
#define _DS_SMP_SCR_DEF 0
#define _DS_SWAPCHAIN_IMAGE 0

#define _FIXME_1_ rgba8

const uint PASS_HDR_1       = 0x00000001u;
const uint PASS_HDR_2       = 0x00000002u;
const uint PASS_EDGE        = 0x00010000u;
const uint PASS_BLUR_1      = 0x00020000u;
const uint PASS_BLUR_2      = 0x00040000u;

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    uint flags;
} pushConstantsBlock;

layout(set=_DS_STR_SCR_CMP_PSTPRC, binding=0) buffer PostProcessDefault {
    // [0] accumulated luminance
    // [1] image size
    uvec4 uData0;
    // [0-3] gaussian weights
    vec4  fData0;
    // [0] 5th gaussian weight
    // [1] edge threshold
    // [2] log average luminance for HDR
    vec4  fData1;   // rem [3]
} postProcess;

layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;
layout(set=_DS_SWAPCHAIN_IMAGE, binding=0, _FIXME_1_) uniform image2D imgTarget;

// IN
layout(local_size_x=1, local_size_y=1) in;

const vec3 LUMINANCE = vec3(0.2126, 0.7152, 0.0722);
float luminance(vec3 color) {
    return dot(LUMINANCE, color);
}

vec4 edge() {
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    float s00 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2(-1,  1)).rgb);
    float s10 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2(-1,  0)).rgb);
    float s20 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2(-1, -1)).rgb);
    float s01 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 0,  1)).rgb);
    float s21 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 0, -1)).rgb);
    float s02 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 1,  1)).rgb);
    float s12 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 1,  0)).rgb);
    float s22 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 1, -1)).rgb);
    // float s00 = luminance(imageLoad(imgTarget, pix + ivec2(-1,  1)).rgb);
    // float s10 = luminance(imageLoad(imgTarget, pix + ivec2(-1,  0)).rgb);
    // float s20 = luminance(imageLoad(imgTarget, pix + ivec2(-1, -1)).rgb);
    // float s01 = luminance(imageLoad(imgTarget, pix + ivec2( 0,  1)).rgb);
    // float s21 = luminance(imageLoad(imgTarget, pix + ivec2( 0, -1)).rgb);
    // float s02 = luminance(imageLoad(imgTarget, pix + ivec2( 1,  1)).rgb);
    // float s12 = luminance(imageLoad(imgTarget, pix + ivec2( 1,  0)).rgb);
    // float s22 = luminance(imageLoad(imgTarget, pix + ivec2( 1, -1)).rgb);

    float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
    float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);

    float g = sx * sx + sy * sy;

    if(g > postProcess.fData1[1])
        return vec4(1.0);
    else
        return vec4(0.0, 0.0, 0.0, 1.0);
    // return texelFetch(sampRender, ivec2(gl_GlobalInvocationID.xy), 0);
}

vec4 blurPass1() {
    // ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    // vec4 sum =   texelFetch(sampRender, pix, 0)               * postProcess.fData0[0];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  1)) * postProcess.fData0[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -1)) * postProcess.fData0[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  2)) * postProcess.fData0[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -2)) * postProcess.fData0[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  3)) * postProcess.fData0[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -3)) * postProcess.fData0[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  4)) * postProcess.fData1[0];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -4)) * postProcess.fData1[0];
    // return sum;
    return vec4(0.0);
}

vec4 blurPass2() {
    // ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    // vec4 sum =   texelFetch(sampRender, pix, 0)               * postProcess.fData0[0];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 1, 0)) * postProcess.fData0[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-1, 0)) * postProcess.fData0[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 2, 0)) * postProcess.fData0[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-2, 0)) * postProcess.fData0[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 3, 0)) * postProcess.fData0[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-3, 0)) * postProcess.fData0[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 4, 0)) * postProcess.fData1[0];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-4, 0)) * postProcess.fData1[0];
    // return sum;
    return vec4(0.0);
}

const vec3 colorSpaceTransform = { 0.2126f, 0.7152f, 0.0722f };
void accumulateLuminace(const in vec4 color) {
    // This doesn't work right but I didn't want to come up with a proper solution.
    // See the not in computeLogAverageLuminance.
    float colorDotCST = dot(color.rgb, colorSpaceTransform);
    colorDotCST = clamp(colorDotCST, 0.00001f, 1.0f);
    uint lum = uint(log(colorDotCST) * -10.0f);
    atomicAdd(postProcess.uData0[0], lum);
}

void computeLogAverageLuminance() {
    // This is some funky math to convert back into color terms
    postProcess.fData1[2] = float(postProcess.uData0[0]) / -10.0f;
    // Average per texel
    postProcess.fData1[2] = postProcess.fData1[2] / float(postProcess.uData0[1]);
    // Log average luminance finally.
    postProcess.fData1[2] = exp(postProcess.fData1[2]);

    // Note: This does not work correctly. I wanted to try to do the math on the GPU
    // to see if I could. I had to try some tricks to get around GPUs not having atomic
    // operations for floating point values. In hindsight, I could probably get accurate
    // numbers by splitting the texture up into sections and running multiple dispatches
    // concurrently in a loop. You could figure out what is the largest section of work
    // you could spilt the operation by where a signicant loss of precision would not
    // occur. That way you can do all the math on the GPU, in what appears to be a pretty
    // fast way with a couple of pipeline barriers.
}

const mat3 rgb2xyz = mat3( 
  0.4124564, 0.2126729, 0.0193339,
  0.3575761, 0.7151522, 0.1191920,
  0.1804375, 0.0721750, 0.9503041 );

const mat3 xyz2rgb = mat3(
  3.2404542, -0.9692660, 0.0556434,
  -1.5371385, 1.8760108, -0.2040259,
  -0.4985314, 0.0415560, 1.0572252 );

const float Exposure = 0.35;
const float White = 0.928;

void main() {
    vec4 color;

    // COLOR
    if ((pushConstantsBlock.flags & PASS_EDGE) > 0) {
        color = edge();
    } else {
        // color = texture2D(sampRender, ivec2(gl_GlobalInvocationID.xy));
        // color = texelFetch(sampRender, ivec2(gl_GlobalInvocationID.xy), 0);
        // vec4(0.0, 1.0, 0.0, 1.0);
    }

    // BLUR
    if ((pushConstantsBlock.flags & PASS_BLUR_1) > 0) {
        color = blurPass1();
    } else if ((pushConstantsBlock.flags & PASS_BLUR_2) > 0) {
        color = blurPass2();
    }

    // HDR
    if ((pushConstantsBlock.flags & PASS_HDR_1) > 0) {
        accumulateLuminace(color);
        // return;
    } else if ((pushConstantsBlock.flags & PASS_HDR_2) > 0) {
        computeLogAverageLuminance();
        return;
    }

    imageStore(imgTarget, ivec2(gl_GlobalInvocationID.xy), color);
}