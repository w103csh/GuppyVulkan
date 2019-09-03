#version 450

// UNIFORMS
#define _DS_UNI_DFR_COMB 0
// SAMPLERS
#define _DS_SMP_SHDW 0
// ATTACHMENTS
#define _DS_SMP_DFR_POS 0
#define _DS_SMP_DFR_NORM 0
#define _DS_SMP_DFR_DIFF 0
#define _DS_SMP_DFR_AMB 0
#define _DS_SMP_DFR_SPEC 0
// #define _DS_SMP_DFR_SSAO 0
// UNIFORM ARRAY SIZES
#define _U_LGT_DEF_POS 0
#define _U_LGT_DEF_SPT 0
#define _U_LGT_SHDW_POS 0

const uint LIGHT_SHOW       = 0x00000001u;

layout(set=_DS_SMP_SHDW, binding=0) uniform sampler2DArrayShadow sampShadow;

layout(input_attachment_index=2, set=_DS_SMP_DFR_POS,   binding=0) uniform subpassInput posInput;
layout(input_attachment_index=3, set=_DS_SMP_DFR_NORM,  binding=0) uniform subpassInput normInput;
layout(input_attachment_index=4, set=_DS_SMP_DFR_DIFF,  binding=0) uniform subpassInput diffInput;
layout(input_attachment_index=5, set=_DS_SMP_DFR_AMB,   binding=0) uniform subpassInput ambInput;
layout(input_attachment_index=6, set=_DS_SMP_DFR_SPEC,  binding=0) uniform subpassInput specInput;
// layout(input_attachment_index=7, set=_DS_SMP_DFR_SSAO,  binding=0) uniform subpassInput ssaoDataInput;

// IN
layout(location=0) in vec2 inTexCoord;
// OUT
layout(location=0) out vec4 outColor;

#if _U_LGT_DEF_POS
layout(set=_DS_UNI_DFR_COMB, binding=1) uniform LightDefaultPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    // 16
    vec3 La;        // Ambient light intesity
    // rem 4
    vec3 L;         // Diffuse and specular light intensity
    // rem 4
} lgtPos[_U_LGT_DEF_POS];

vec3 blinnPhongPositional(
    vec3 pos,
    vec3 norm,
    vec3 v,
    vec3 diff,
    vec3 amb,
    vec3 spec,
    float shininess
    ) {

    vec3 color = vec3(0.0);

    for (int i = 0; i < lgtPos.length(); i++) {
        if ((lgtPos[i].flags & LIGHT_SHOW) > 0) {

            vec3 Ka = pow((lgtPos[i].La * amb), vec3(i+1));
            vec3 s = normalize(lgtPos[i].position - pos);

            float sDotN = max(dot(s, norm), 0.0);
            vec3 Kd = diff * sDotN;

            vec3 Ks = vec3(0.0);
            if(sDotN > 0.0) {
                vec3 h = normalize(v + s);
                Ks = spec * pow(max(dot(h, norm), 0.0), shininess);
            }

            color += Ka + lgtPos[i].L * (Kd + Ks);
        }
    }

    return color;

}
#endif

#if _U_LGT_DEF_SPT
layout(set=_DS_UNI_DFR_COMB, binding=2) uniform LightDefaultSpot {
    vec3 position;
    uint flags;
    // 16
    vec3 La;
    float exponent;
    // 16
    vec3 L;
    float cutoff;
    // 16
    vec3 direction;
    // rem 4
} lgtSpot[_U_LGT_DEF_SPT];

vec3 blinnPhongSpot(
    vec3 pos,
    vec3 norm,
    vec3 v,
    vec3 diff,
    vec3 amb,
    vec3 spec,
    float shininess
    ) {

    vec3 color = vec3(0.0);

    for (int i = 0; i < lgtSpot.length(); i++) {
        if ((lgtSpot[i].flags & LIGHT_SHOW) > 0) {

            vec3 s = normalize(lgtSpot[i].position - pos);
            float cosAng = dot(-s, normalize(lgtSpot[i].direction));
            float angle = acos(cosAng);

            if(angle < lgtSpot[i].cutoff) {

                float spotScale = pow(cosAng, lgtSpot[i].exponent);
                float sDotN = max(dot(s, norm), 0.0);
                vec3 Kd = diff * sDotN;

                vec3 Ks = vec3(0.0);
                if (sDotN > 0.0) {
                    vec3 h = normalize(v + s);
                    Ks = spec * pow(max(dot(h, norm), 0.0), shininess);
                }

                color += lgtSpot[i].La + spotScale * lgtSpot[i].L * (Kd + Ks);
            }
        }
    }

    return color;

}
#endif

#if _U_LGT_SHDW_POS
layout(set=_DS_UNI_DFR_COMB, binding=3) uniform LightShadowPositional {
    mat4 proj;
} lgtShadowPos[_U_LGT_SHDW_POS];

const float eps = 0.1;
bool epsilonEqual(float f1, float f2) {
    return abs(f1 - f2) < eps;
}
bool epsilonEqual3(vec3 v1, vec3 v2) {
    bvec3 comp = bvec3(
        (abs(v1.x - v2.x) < eps),
        (abs(v1.y - v2.y) < eps),
        (abs(v1.z - v2.z) < eps)
    );
    return all(comp);
}
bool epsilonEqual4(vec4 v1, vec4 v2) {
    bvec4 comp = bvec4(
        (abs(v1.x - v2.x) < eps),
        (abs(v1.y - v2.y) < eps),
        (abs(v1.z - v2.z) < eps),
        (abs(v1.w - v2.w) < eps)
    );
    return all(comp);
}

vec3 getShadowTexCoord3(const in mat4 m, const in vec3 pos) {
    vec4 tc = lgtShadowPos[0].proj * vec4(pos, 1.0);
    return vec3(tc.x / tc.w, tc.y / tc.w, tc.z / tc.w);
}

vec4 getShadowTexCoord4(const in mat4 m, const in vec3 pos, const in int arrayLayer) {
    vec4 tc = lgtShadowPos[0].proj * vec4(pos, 1.0);
    tc.x /= tc.w;
    tc.y /= tc.w;
    tc.w = tc.z / tc.w;
    tc.z = arrayLayer;
    return tc;
}

vec3 blinnPhongPositionalShadow(
    vec3 pos,
    vec3 norm,
    vec3 v,
    vec3 diff,
    vec3 spec,
    float shininess
    ) {

    vec3 color = vec3(0.0);

    for (int i = 0; i < lgtPos.length(); i++) {
        if ((lgtPos[i].flags & LIGHT_SHOW) > 0) {
            if (i > 0) {
                // color += vec3(1, 0, 0);
                continue;
            }

            if (distance(pos, lgtPos[i].position) > 20) continue;

            vec3 s = normalize(lgtPos[i].position - pos);

            float sDotN = max(dot(s, norm), 0.0);
            vec3 Kd = diff * sDotN;

            vec3 Ks = vec3(0.0);
            if(sDotN > 0.0) {
                vec3 h = normalize(v + s);
                Ks = spec * pow(max(dot(h, norm), 0.0), shininess);
            }

            float shadow = 1.0;
            {
                /* Shadow texture coords (This was a real pain in the ass to figure out!):
                 *
                 *  sampler2DArrayShadow:
                 *      x: s
                 *      y: t
                 *      z: array layer
                 *      w: D(ref) for depth comparison
                 *
                 *  sampler2DShadow:
                 *      x: s
                 *      y: t
                 *      z: D(ref) for depth comparison
                 *
                 * Notice that the z value changes between the different sampler types this took
                 *  hours to figure out. Hopefully you remember this.
                 *
                 * Also, if sampler filtering is enabled then the result of a texture lookup is
                 *  non-binary. If, for example, half of the samples pass the depth test then 0.5
                 *  is returned.
                 */
                vec4 shadowTexCoord = getShadowTexCoord4(lgtShadowPos[0].proj, pos, 0);

                if (shadowTexCoord.w >= 0) { // w is the depth reference for sampler2DArrayShadow

                    // TODO: add a proper PCF flag. This should probably so through all the way to
                    // VkSamplerCreateInfo (nearest vs. linear).

                    if (false) {
                        // Normal
                        shadow = texture(sampShadow, shadowTexCoord);
                    } else {
                        // Percentage-closer fliltering (PCF) shadows 
                        float sum = 0.0;
                        sum += textureOffset(sampShadow, shadowTexCoord, ivec2(-1, -1));
                        sum += textureOffset(sampShadow, shadowTexCoord, ivec2(-1,  1));
                        sum += textureOffset(sampShadow, shadowTexCoord, ivec2( 1,  1));
                        sum += textureOffset(sampShadow, shadowTexCoord, ivec2( 1, -1));
                        // shadow = sum * 0.25; // I think it looks better with the actual sampler too.
                        sum += texture(sampShadow, shadowTexCoord);
                        shadow = sum * 0.2;
                    }

                }
            }

            color += lgtPos[i].L * (Kd + Ks) * shadow;
        }
    }

    return color;

}
#endif

void main() {
    vec3 pos = subpassLoad(posInput).xyz;
    vec3 v = normalize(-pos);
    vec4 norm = subpassLoad(normInput);
    vec4 diff = subpassLoad(diffInput);
    vec4 amb = subpassLoad(ambInput);
    vec4 spec = subpassLoad(specInput);
    // float ssaoData = subpassLoad(ssaoDataInput)[0];

    // outColor = diff;
    // return;

#if _U_LGT_SHDW_POS

    // vec3 Ka = 0.9 * amb.rgb; // amb has no color
    float La = 0.2;
    vec3 Ka = La * mix(amb.rgb, diff.rgb, 0.5); // amb has no color
    vec3 KdKs = blinnPhongPositionalShadow(
        pos,
        norm.xyz,
        v,
        diff.rgb,
        spec.rgb,
        norm.w
    );

    outColor = vec4((Ka + KdKs), diff.a);

#else

    outColor = vec4(
        blinnPhongPositional(
            pos,
            norm.xyz,
            v,
            diff.rgb,
            amb.rgb,
            spec.rgb,
            norm.w
            )
        + blinnPhongSpot(
            pos,
            norm.xyz,
            v,
            diff.rgb,
            amb.rgb,
            spec.rgb,
            norm.w
            )
        ,diff.a
    );

#endif
}