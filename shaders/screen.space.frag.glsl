
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_UNI_SCR_DEF 0
#define _DS_SMP_SCR_DEF 0

// DECLARATIONS
bool isScreenSpaceBlurPass1();
bool isScreenSpaceBlurPass2();
bool isScreenSpaceEdge();

layout(set=_DS_UNI_SCR_DEF, binding=0, std140) uniform ScreenSpaceDefault {
    vec4 weights0_3;                // gaussian normalized weights[0-3]
    float weight4;                  // gaussian normalized weights[4]
    float edgeThreshold;            //
    // 8 rem
} data;

layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;

// OUT
layout(location=0) out vec4 outColor;


const vec3 LUMINANCE = vec3(0.2126, 0.7152, 0.0722);
float luminance(vec3 color) {
    return dot(LUMINANCE, color);
}

vec4 blurPass1() {
    ivec2 pix = ivec2(gl_FragCoord.xy);
    vec4 sum =   texelFetch(sampRender, pix, 0)               * data.weights0_3[0];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  1)) * data.weights0_3[1];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -1)) * data.weights0_3[1];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  2)) * data.weights0_3[2];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -2)) * data.weights0_3[2];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  3)) * data.weights0_3[3];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -3)) * data.weights0_3[3];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0,  4)) * data.weight4;
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(0, -4)) * data.weight4;
    return sum;
    return vec4(0.0);
}

vec4 blurPass2() {
    ivec2 pix = ivec2(gl_FragCoord.xy);
    vec4 sum =   texelFetch(sampRender, pix, 0)               * data.weights0_3[0];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2( 1, 0)) * data.weights0_3[1];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(-1, 0)) * data.weights0_3[1];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2( 2, 0)) * data.weights0_3[2];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(-2, 0)) * data.weights0_3[2];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2( 3, 0)) * data.weights0_3[3];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(-3, 0)) * data.weights0_3[3];
    sum += texelFetchOffset(sampRender, pix, 0, ivec2( 4, 0)) * data.weight4;
    sum += texelFetchOffset(sampRender, pix, 0, ivec2(-4, 0)) * data.weight4;
    return sum;
    return vec4(0.0);
}

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
    // if(g > data.thing2[1])
        return vec4(1.0);
    else
        return vec4(0.0, 0.0, 0.0, 1.0);
}

void main() {
    if (isScreenSpaceBlurPass1()) {
        outColor = blurPass1();
        // outColor = vec4(1.0, 0.0, 1.0, 1.0);
    } else if (isScreenSpaceBlurPass2()) {
        outColor = blurPass2();
        // outColor = vec4(1.0, 1.0, 0.0, 1.0);
    } else if (isScreenSpaceEdge()) {
        outColor = edge();
    } else {
        // test
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
}