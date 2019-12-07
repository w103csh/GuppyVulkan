/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _U_LGT_DEF_POS 0

#if _U_LGT_DEF_SPT
layout(set=_DS_UNI_DFR_COMB, binding=4) uniform LightDefaultSpot {
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
#if _U_LGT_DEF_POS
    color += blinnPhongPositional(
        pos.xyz,
        norm.xyz,
        v,
        diff.rgb,
        amb.rgb,
        spec.rgb,
        norm.w
        );
#endif
}
