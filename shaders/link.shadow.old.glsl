/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_SHDW_POS_ALL 0
#define _U_LGT_SHDW_POS 0

layout(set=_DS_SHDW_POS_ALL, binding=0) uniform sampler2DArrayShadow sampShadow;

#if _U_LGT_SHDW_POS
layout(set=_DS_SHDW_POS_ALL, binding=4) uniform LightShadowPositional {
    mat4 proj;
} lgtShadowPos[_U_LGT_SHDW_POS];
layout(set=_DS_SHDW_POS_ALL, binding=5) uniform ShadowDefault {
    vec4 data;
} shadowData;

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
    vec4 tc = m * vec4(pos, 1.0);
    return vec3(tc.x / tc.w, tc.y / tc.w, tc.z / tc.w);
}

vec4 getShadowTexCoord4(const in mat4 m, const in vec3 pos, const in int arrayLayer) {
    vec4 tc = m * vec4(pos, 1.0);
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
            vec4 shadowTexCoord = getShadowTexCoord4(lgtShadowPos[i].proj, pos, 0);

            if (false) { // This technique appears to rely on the original depth attachment to be sampled.

                float radius = shadowData.data.w * 1.0;

                ivec3 offsetCoord;
                offsetCoord.xy = ivec2( mod( inTexCoord.xy, shadowData.data.xy ) );

                float sum = 0.0, shadow = 1.0;
                int samplesDiv2 = int(shadowData.data.z);
                vec4 sc = shadowTexCoord;

                // Don't test points behind the light source.
                if ( sc.z >= 0 ) {
                    for( int i = 0 ; i < 4; i++ ) {
                        offsetCoord.z = i;
                        vec4 offsets = texelFetch(sampShadowOffset, offsetCoord, 0) * radius;

                        sc.xy = shadowTexCoord.xy + offsets.xy;
                        sum += texture(sampShadow, sc);
                        sc.xy = shadowTexCoord.xy + offsets.zw;
                        sum += texture(sampShadow, sc);
                    }
                    shadow = sum / 8.0;
                    
                    if( shadow != 1.0 && shadow != 0.0 ) {
                        for( int i = 4; i < samplesDiv2; i++ ) {
                            offsetCoord.z = i;
                            vec4 offsets = texelFetch(sampShadowOffset, offsetCoord, 0) * radius; // * w;

                            sc.xy = shadowTexCoord.xy + offsets.xy;
                            sum += texture(sampShadow, sc);
                            sc.xy = shadowTexCoord.xy + offsets.zw;
                            sum += texture(sampShadow, sc);
                        }
                        shadow = sum / float(samplesDiv2 * 2.0);
                    }

                }

                color += lgtPos[i].L * (Kd + Ks) * shadow;

            } else {
                float shadow = 1.0;

                if (shadowTexCoord.w >= 0) { // w is the depth reference for sampler2DArrayShadow

                    // TODO: add a proper PCF flag. This should probably so through all the way to
                    // VkSamplerCreateInfo (nearest vs. linear).

                    if (true) {
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

                        ivec3 offsetCoord;
                        offsetCoord.xy = ivec2(mod(gl_FragCoord.xy, ivec2(8, 8))); // !

                        offsetCoord.z = 0;
                        vec4 offsets = texelFetch(sampShadowOffset, offsetCoord, 0) * (7.0/215.0) * shadowTexCoord.w;
                    }

                }

                color += lgtPos[i].L * (Kd + Ks) * shadow;
            }

        }
    }

    return color;

}
#endif

// This was what the calling function looked like...
void main() {
    vec3 color = vec3(0);
#elif _U_LGT_SHDW_POS
    // vec3 Ka = 0.9 * amb.rgb; // amb has no color
    float La = 0.2;
    vec3 Ka = La * mix(amb.rgb, diff.rgb, 0.5); // amb has no color
    vec3 KdKs = blinnPhongPositionalShadow(
        pos.xyz,
        norm.xyz,
        v,
        diff.rgb,
        spec.rgb,
        norm.w
    );
    color += (Ka + KdKs);
#endif

}
