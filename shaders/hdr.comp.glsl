
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_STR_SCR_CMP_PSTPRC 0
#define _DS_SMP_SCR_DEF 0
#define _DS_STRIMG_SCR_CMP_DEF 0

#define _FIXME_1_ rgba8

const uint PASS_1       = 0x00000001u;
const uint PASS_2       = 0x00000002u;

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    uint flags;
} pushConstantsBlock;

layout(set=_DS_STR_SCR_CMP_PSTPRC, binding=0) buffer PostProcessDefault {
    // [0] accumulated luminance
    // [1] image size
    uvec4 uData1;
    // [0] log average luminance for HDR
    vec4  fData1;
} postProcess;

layout(set=_DS_SMP_SCR_DEF, binding=0) uniform sampler2D sampRender;
layout(set=_DS_STRIMG_SCR_CMP_DEF, binding=0, _FIXME_1_) uniform image2D outImage;

// IN
layout (local_size_x=1, local_size_y=1) in;

const vec3 colorSpaceTransform = { 0.2126f, 0.7152f, 0.0722f };
void accumulateLuminace(const in vec4 color) {
    // This doesn't work right but I didn't want to come up with a proper solution.
    // See the not in computeLogAverageLuminance.
    float colorDotCST = dot(color.rgb, colorSpaceTransform);
    colorDotCST = clamp(colorDotCST, 0.00001f, 1.0f);
    uint lum = uint(log(colorDotCST) * -10.0f);
    atomicAdd(postProcess.uData1[0], lum);
}

void computeLogAverageLuminance() {
    // This is some funky math to convert back into color terms
    postProcess.fData1[0] = float(postProcess.uData1[0]) / -10.0f;
    // Average per texel
    postProcess.fData1[0] = postProcess.fData1[0] / float(postProcess.uData1[1]);
    // Log average luminance finally.
    postProcess.fData1[0] = exp(postProcess.fData1[0]);

    // Note: This does not work correctly. I wanted to try to do the math on the GPU
    // to see if I could. I had to try some tricks to get around GPUs not having atomic
    // operations for floating point values. In hindsight, I could probably get accurate
    // numbers by splitting the texture up into sections and running multiple dispatches
    // concurrently in a loop. You could figure out what is the largest section of work
    // you could spilt the operation by where a signicant loss of precision would not
    // occur. That way you can do all the math on the GPU, in what appears to be a pretty
    // fast way with a couple of pipeline barriers.
}

void main() {
    if ((pushConstantsBlock.flags & PASS_1) > 0) {
        vec4 color = texelFetch(sampRender, ivec2(gl_GlobalInvocationID.xy), 0);
        accumulateLuminace(color);
        imageStore(outImage, ivec2(gl_GlobalInvocationID.xy), color);
    } else if ((pushConstantsBlock.flags & PASS_2) > 0) {
        computeLogAverageLuminance();
    }
}