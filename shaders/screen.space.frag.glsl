
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_UNI_SCR_DEF 0
#define _DS_SMP_SCR_DEF 0

#define _DS_STR_SCR_CMP_PSTPRC 0
#define _DS_STRIMG_SCR_CMP_DEF 0
#define _FIXME_1_ rgba8

// DECLARATIONS
bool isScreenSpaceBlurPass1();
bool isScreenSpaceBlurPass2();
bool isScreenSpaceEdge();

layout(set=_DS_UNI_SCR_DEF, binding=0) uniform ScreenSpaceDefault {
    vec4 weights0_3;                // gaussian normalized weights[0-3]
    float weight4;                  // gaussian normalized weights[4]
    float edgeThreshold;            //
    // 8 rem
} data;

layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;

layout(set=_DS_STR_SCR_CMP_PSTPRC, binding=0) buffer readonly PostProcessDefault {
    // [0] accumulated luminance
    // [1] image size
    uvec4 uData1;
    // [0] log average luminance for HDR
    vec4  fData1;
} postProcess;
layout(set=_DS_STRIMG_SCR_CMP_DEF, binding=0, _FIXME_1_) uniform readonly image2D inImage;

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
    /////////////// Tone mapping ///////////////
    // Retrieve high-res color from texture
    vec4 color = texelFetch(sampRender, ivec2(gl_FragCoord.xy), 0);

    // Convert to XYZ
    vec3 xyzCol = rgb2xyz * vec3(color);

    // Convert to xyY
    float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
    vec3 xyYCol = vec3( xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);

    // Apply the tone mapping operation to the luminance (xyYCol.z or xyzCol.y)
    float L = (Exposure * xyYCol.z) / postProcess.fData1[0];
    L = (L * ( 1 + L / (White * White) )) / ( 1 + L );

    // Using the new luminance, convert back to XYZ
    xyzCol.x = (L * xyYCol.x) / (xyYCol.y);
    xyzCol.y = L;
    xyzCol.z = (L * (1 - xyYCol.x - xyYCol.y))/xyYCol.y;

    // Convert back to RGB
    outColor = vec4( xyz2rgb * xyzCol, 1.0);
    return;

    outColor = texelFetch(sampRender, ivec2(gl_FragCoord.xy), 0);
    // return;

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