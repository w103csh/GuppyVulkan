/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_SCR_DEF 0
#define _DS_SMP_SCR_DEF -1
#define _DS_SMP_SCR_HDR_LOG_BLIT -1
#define _DS_SMP_SCR_BLUR_A -1

const uint PASS_HDR_1               = 0x00000001u;
const uint PASS_HDR_2               = 0x00000002u;
const uint PASS_EDGE                = 0x00010000u;
const uint PASS_BLUR_A              = 0x00020000u;
const uint PASS_BLUR_B              = 0x00040000u;
const uint PASS_BLOOM_BRIGHT        = 0x00100000u;
const uint PASS_BLOOM_BLUR_A        = 0x00200000u;
const uint PASS_BLOOM_BLUR_B        = 0x00400000u;
const uint PASS_BLOOM               = 0x00800000u;

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    uint flags;
} pushConstantsBlock;

layout(set=_DS_UNI_SCR_DEF, binding=0) uniform ScreenSpaceDefault {
    vec4  weights0_3;               // gaussian normalized weights[0-3]
    vec4  weights4_7;               // gaussian normalized weights[4-7]
    vec2  weights8_9;               // gaussian normalized weights[8-9]
    float edgeThreshold;            //
    float luminanceThreshold;       //
    // 4 rem
} data;

#if _DS_SMP_SCR_DEF >= 0
layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;
#endif
#if _DS_SMP_SCR_HDR_LOG_BLIT >= 0
layout(set=_DS_SMP_SCR_HDR_LOG_BLIT, binding=0) uniform sampler2D hdrBlit;
#endif
#if _DS_SMP_SCR_BLUR_A >= 0
layout(set=_DS_SMP_SCR_BLUR_A, binding=0) uniform sampler2D sampBlurA;
#endif


// IN
layout(location=0) in vec2 fragTexCoord;
// OUT
layout(location=0) out vec4 outColor;

const vec3 LUMINANCE = vec3(0.2126, 0.7152, 0.0722);
float luminance(const in vec3 color) {  return dot(LUMINANCE, color); }

vec4 blurPass1() {
    // The weights changed!
    // ivec2 pix = ivec2(gl_FragCoord.xy);
    // vec4 sum =   texelFetch(sampRender, pix, 0)               * data.weights0_3[0];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  1)) * data.weights0_3[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -1)) * data.weights0_3[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  2)) * data.weights0_3[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -2)) * data.weights0_3[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  3)) * data.weights0_3[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -3)) * data.weights0_3[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  4)) * data.weight4;
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -4)) * data.weight4;
    // return sum;
    return vec4(0.0);
}

vec4 blurPass2() {
    // The weights changed!
    // ivec2 pix = ivec2(gl_FragCoord.xy);
    // vec4 sum =   texelFetch(sampRender, pix, 0)               * data.weights0_3[0];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 1, 0)) * data.weights0_3[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-1, 0)) * data.weights0_3[1];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 2, 0)) * data.weights0_3[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-2, 0)) * data.weights0_3[2];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 3, 0)) * data.weights0_3[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-3, 0)) * data.weights0_3[3];
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2( 4, 0)) * data.weight4;
    // sum += texelFetchOffset(sampRender, pix, 0, ivec2(-4, 0)) * data.weight4;
    // return sum;
    return vec4(0.0);
}

#if _DS_SMP_SCR_DEF >= 0
vec4 bright() {
    vec4 color = texture(sampRender, fragTexCoord);
    if( luminance(color.rgb) > data.luminanceThreshold ) {
        return texelFetch(sampRender, ivec2(gl_FragCoord.xy), 0);
    } else {
        return vec4(0.0);
    }
}
#endif

#if _DS_SMP_SCR_DEF >= 0
vec4 edge() {
    ivec2 pix = ivec2(gl_FragCoord.xy);

    float s00 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2(-1,  1)).rgb);
    float s10 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2(-1,  0)).rgb);
    float s20 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2(-1, -1)).rgb);
    float s01 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 0,  1)).rgb);
    float s21 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 0, -1)).rgb);
    float s02 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 1,  1)).rgb);
    float s12 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 1,  0)).rgb);
    float s22 = luminance(texelFetchOffset(sampRender, pix, 0, ivec2( 1, -1)).rgb);

    float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
    float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);

    float g = sx * sx + sy * sy;

    if(g > data.edgeThreshold)
        return vec4(1.0);
    else
        return vec4(0.0, 0.0, 0.0, 1.0);
}
#endif

#if _DS_SMP_SCR_HDR_LOG_BLIT >= 0
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
vec4 getToneMapColor() {
    float AveLum = exp(texelFetch(hdrBlit, ivec2(0, 0), 9).r);

    /////////////// Tone mapping ///////////////
    // Retrieve high-res color from texture
    vec4 color = texture(sampRender, fragTexCoord);
    
    // Convert to XYZ
    vec3 xyzCol = rgb2xyz * vec3(color);

    // Convert to xyY
    float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
    vec3 xyYCol = vec3( xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);

    // Apply the tone mapping operation to the luminance (xyYCol.z or xyzCol.y)
    float L = (Exposure * xyYCol.z) / AveLum;
    L = (L * ( 1 + L / (White * White) )) / ( 1 + L );

    // Using the new luminance, convert back to XYZ
    xyzCol.x = (L * xyYCol.x) / (xyYCol.y);
    xyzCol.y = L;
    xyzCol.z = (L * (1 - xyYCol.x - xyYCol.y))/xyYCol.y;

    // Convert back to RGB
    return vec4( xyz2rgb * xyzCol, 1.0);
}
#endif

void main() {
    // // HDR
    // if ((pushConstantsBlock.flags & PASS_HDR_1) > 0) {
    //     accumulateLuminace(color);
    //     // return;
    // } else if ((pushConstantsBlock.flags & PASS_HDR_2) > 0) {
    //     computeLogAverageLuminance();
    //     return;
    // }

    // BLUR
    if ((pushConstantsBlock.flags & PASS_BLUR_A) > 0) {
        outColor = blurPass1();
    } else if ((pushConstantsBlock.flags & PASS_BLUR_B) > 0) {
        outColor = blurPass2();
    }

    // BRIGHT
    if ((pushConstantsBlock.flags & PASS_BLOOM_BRIGHT) > 0) {
#if _DS_SMP_SCR_DEF > 0
        outColor = bright();
#endif
        return;
    }

    // COLOR
#if _DS_SMP_SCR_DEF >= 0
    if ((pushConstantsBlock.flags & PASS_EDGE) > 0) {
        outColor = edge();
    } else {
#if _DS_SMP_SCR_HDR_LOG_BLIT >= 0
        outColor = getToneMapColor();
#else
        outColor = texelFetch(sampRender, ivec2(gl_FragCoord.xy), 0);
#endif
    }
#endif

    // Add bloom to HDR
    if ((pushConstantsBlock.flags & PASS_BLOOM) > 0) {
#if _DS_SMP_SCR_BLUR_A >= 0
        outColor += texture(sampBlurA, fragTexCoord);
#endif
    }
}