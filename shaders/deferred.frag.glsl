#version 450

// UNIFORMS
#define _DS_UNI_DFR_COMB 0
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

const uint LIGHT_SHOW       = 0x00000001u;

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
}