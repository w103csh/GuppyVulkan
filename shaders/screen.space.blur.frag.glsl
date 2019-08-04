
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _DS_UNI_SCR_DEF 0
#define _DS_SMP_SCR_BLUR_A -1
#define _DS_SMP_SCR_BLUR_B -1

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

#if _DS_SMP_SCR_BLUR_A >= 0
layout(set=_DS_SMP_SCR_BLUR_A, binding=0) uniform sampler2D sampBlurA;
#endif
#if _DS_SMP_SCR_BLUR_B >= 0
layout(set=_DS_SMP_SCR_BLUR_B, binding=0) uniform sampler2D sampBlurB;
#endif

// IN
layout(location=0) in vec2 fragTexCoord;
// OUT
layout(location=0) out vec4 outColor;

#if _DS_SMP_SCR_BLUR_A >= 0
vec4 blurPass1() {
    float dy = 1.0 / textureSize(sampBlurA, 0).y;
    vec3 sum = texture(sampBlurA, fragTexCoord).rgb * data.weights0_3[0];
    {
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 1.0) * dy).rgb * data.weights0_3[1];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 1.0) * dy).rgb * data.weights0_3[1];
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 2.0) * dy).rgb * data.weights0_3[2];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 2.0) * dy).rgb * data.weights0_3[2];
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 3.0) * dy).rgb * data.weights0_3[3];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 3.0) * dy).rgb * data.weights0_3[3];
    }
    {
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 4.0) * dy).rgb * data.weights4_7[0];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 4.0) * dy).rgb * data.weights4_7[0];
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 5.0) * dy).rgb * data.weights4_7[1];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 5.0) * dy).rgb * data.weights4_7[1];
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 6.0) * dy).rgb * data.weights4_7[2];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 6.0) * dy).rgb * data.weights4_7[2];
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 7.0) * dy).rgb * data.weights4_7[3];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 7.0) * dy).rgb * data.weights4_7[3];
    }
    {
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 8.0) * dy).rgb * data.weights8_9[0];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 8.0) * dy).rgb * data.weights8_9[0];
        sum += texture(sampBlurA, fragTexCoord + vec2(0.0, 9.0) * dy).rgb * data.weights8_9[1];
        sum += texture(sampBlurA, fragTexCoord - vec2(0.0, 9.0) * dy).rgb * data.weights8_9[1];
    }
    return vec4(sum, 1.0);
}
#endif

#if _DS_SMP_SCR_BLUR_B >= 0
vec4 blurPass2() {
    float dx = 1.0 / textureSize(sampBlurB, 0).x;
    vec3 sum = texture(sampBlurB, fragTexCoord).rbg * data.weights0_3[0];
    {
        sum += texture(sampBlurB, fragTexCoord + vec2(1.0, 0.0) * dx).rgb * data.weights0_3[1];
        sum += texture(sampBlurB, fragTexCoord - vec2(1.0, 0.0) * dx).rgb * data.weights0_3[1];
        sum += texture(sampBlurB, fragTexCoord + vec2(2.0, 0.0) * dx).rgb * data.weights0_3[2];
        sum += texture(sampBlurB, fragTexCoord - vec2(2.0, 0.0) * dx).rgb * data.weights0_3[2];
        sum += texture(sampBlurB, fragTexCoord + vec2(3.0, 0.0) * dx).rgb * data.weights0_3[3];
        sum += texture(sampBlurB, fragTexCoord - vec2(3.0, 0.0) * dx).rgb * data.weights0_3[3];
    }
    {
        sum += texture(sampBlurB, fragTexCoord + vec2(4.0, 0.0) * dx).rgb * data.weights4_7[0];
        sum += texture(sampBlurB, fragTexCoord - vec2(4.0, 0.0) * dx).rgb * data.weights4_7[0];
        sum += texture(sampBlurB, fragTexCoord + vec2(5.0, 0.0) * dx).rgb * data.weights4_7[1];
        sum += texture(sampBlurB, fragTexCoord - vec2(5.0, 0.0) * dx).rgb * data.weights4_7[1];
        sum += texture(sampBlurB, fragTexCoord + vec2(6.0, 0.0) * dx).rgb * data.weights4_7[2];
        sum += texture(sampBlurB, fragTexCoord - vec2(6.0, 0.0) * dx).rgb * data.weights4_7[2];
        sum += texture(sampBlurB, fragTexCoord + vec2(7.0, 0.0) * dx).rgb * data.weights4_7[3];
        sum += texture(sampBlurB, fragTexCoord - vec2(7.0, 0.0) * dx).rgb * data.weights4_7[3];
    }
    {
        sum += texture(sampBlurB, fragTexCoord + vec2(8.0, 0.0) * dx).rgb * data.weights8_9[0];
        sum += texture(sampBlurB, fragTexCoord - vec2(8.0, 0.0) * dx).rgb * data.weights8_9[0];
        sum += texture(sampBlurB, fragTexCoord + vec2(9.0, 0.0) * dx).rgb * data.weights8_9[1];
        sum += texture(sampBlurB, fragTexCoord - vec2(9.0, 0.0) * dx).rgb * data.weights8_9[1];
    }
    return vec4(sum, 1.0);
}
#endif

void main() {
    if ((pushConstantsBlock.flags & PASS_BLOOM_BLUR_A) > 0) {
#if _DS_SMP_SCR_BLUR_A >= 0
        outColor = blurPass1();
#endif
    } else if ((pushConstantsBlock.flags & PASS_BLOOM_BLUR_B) > 0) {
#if _DS_SMP_SCR_BLUR_B >= 0
        outColor = blurPass2();
#endif
        // outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
}