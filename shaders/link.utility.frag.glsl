/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _U_LGT_DEF_POS 0
#define _U_LGT_DEF_SPT 0
#define _DS_UNI_DEF 0
#define _DS_SMP_DEF 0
#define _DS_PRJ_DEF -1

// DECLARATIONS
vec3 transform(vec3 v);
vec3 getMaterialAmbient();
vec3 getMaterialColor();
vec3 getMaterialSpecular();
uint getMaterialFlags();
uint getMaterialTexFlags();
float getMaterialOpacity();
float getMaterialXRepeat();
float getMaterialYRepeat();
float getMaterialShininess();
bool isModeToonShade();
vec3 blinnPhongPositional(
    const in vec3 La,
    const in vec3 L,
    const in vec3 s,
    const in float shininess
);
vec3 blinnPhongSpot(
    const in vec3 La,
    const in vec3 L,
    const in vec3 s,
    const in float shininess,
    const in vec3 lightDir,
    const in float cutoff,
    const in float exponent
);

// FLAGS
// LIGHT
const uint LIGHT_SHOW       = 0x00000001u;
// FOG
const uint FOG_LINEAR       = 0x00000001u;
const uint FOG_EXP          = 0x00000002u;
const uint FOG_EXP2         = 0x00000004u;
const uint _FOG_SHOW        = 0x0000000Fu;

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0, std140) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

layout(set=_DS_UNI_DEF, binding=2, std140) uniform UniformDefaultFog {
    float minDistance;
    float maxDistance; 
    float density;
    uint flags;
    // 16
    vec3 color;
    // rem 4
} fog;

#if _U_LGT_DEF_POS
layout(set=_DS_UNI_DEF, binding=3) uniform LightDefaultPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    // 16
    vec3 La;        // Ambient light intesity
    // rem 4
    vec3 L;         // Diffuse and specular light intensity
    // rem 4
} lgtPos[_U_LGT_DEF_POS];
#endif

#if _U_LGT_DEF_SPT
layout(set=_DS_UNI_DEF, binding=4) uniform LightDefaultSpot {
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
#endif

#if _DS_PRJ_DEF > -1
layout(set=_DS_PRJ_DEF, binding=0) uniform sampler2D sampProjector;
#endif

// IN
layout(location=0) in vec3 inPosition;
#if _DS_PRJ_DEF > -1
layout(location=6) in vec4 inProjTexCoord;
layout(location=7) in vec4 inTest;
#endif

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n,      // normal
        v;      // direction to the camera
float opacity, height;


const int ts_levels = 3;
const float ts_scaleFactor = 1.0 / ts_levels;
vec3 toonShade(
    const in vec3 La,
    const in vec3 L,
    const in vec3 s
) {
    vec3 ambient = La * Ka;
    float sDotN = max(dot(s, n), 0.0);
    vec3 diff =  Kd * floor(sDotN * ts_levels) * ts_scaleFactor;
    return ambient + L * diff;
}

float fogFactor() {
    float f = 0.0f;
    // float dist = abs(inPosition.z); // faster version
    float z = length(inPosition.xyz);

    // Each of below has slightly different properties. Read
    // the book again if you want to hear their explanation.
    if ((fog.flags & FOG_LINEAR) > 0) {
        f = (fog.maxDistance - z) / (fog.maxDistance - fog.minDistance);
    }
    else if ((fog.flags & FOG_EXP) > 0) {
        f = exp(-fog.density * z);
    }
    else if ((fog.flags & FOG_EXP2) > 0) {
        f = exp(-pow((fog.density * z), 2));
    }
    return clamp(f, 0.0, 1.0);
}

void addProjectorTexColor(inout vec3 color) {
#if _DS_PRJ_DEF > -1
    if(inProjTexCoord.z > 0.0) {
        vec3 projColor = textureProj(sampProjector, inProjTexCoord).rgb;
        color += projColor * 0.8;
        // if (any(notEqual(projColor, vec3(0.0))))
        //     color = projColor * 0.5;
        // else
        //     color;
    }
#endif
}

vec3 blinnPhongShade() {
    vec3 color = vec3(0.0);
    uint lightCount = 0;
    v = normalize(transform(vec3(0.0) - inPosition));
    float shininess = getMaterialShininess();

#if _U_LGT_DEF_POS
    for (int i = 0; i < lgtPos.length(); i++) {
        if ((lgtPos[i].flags & LIGHT_SHOW) > 0) {

            if (isModeToonShade()) {

                color += toonShade(
                    pow(lgtPos[i].La, vec3(++lightCount)),
                    lgtPos[i].L,
                    normalize(transform(lgtPos[i].position - inPosition))
                );

            } else {

                color += blinnPhongPositional(
                    pow(lgtPos[i].La, vec3(++lightCount)),
                    lgtPos[i].L,
                    normalize(transform(lgtPos[i].position - inPosition)),
                    shininess
                );

            }

        }
    }
#endif

#if _U_LGT_DEF_SPT
    for (int i = 0; i < lgtSpot.length(); i++) {
        if ((lgtSpot[i].flags & LIGHT_SHOW) > 0) {

            color += blinnPhongSpot(
                pow(lgtSpot[i].La, vec3(++lightCount)),
                lgtSpot[i].L,
                normalize(transform(lgtSpot[i].position - inPosition)),
                shininess,
                normalize(transform(lgtSpot[i].direction)),
                lgtSpot[i].cutoff,
                lgtSpot[i].exponent
            );

        }
    }
#endif

    if ((fog.flags & _FOG_SHOW) > 0) {
        color = mix(fog.color, color, fogFactor());
    }

    addProjectorTexColor(color);

    return color;
}

const float GAMMA = 2.2;
vec4 gammaCorrect(const in vec3 color, const in float opacity) {
    if (false)
        return vec4(pow(color, vec3(1.0/GAMMA)), opacity);
    else
        return vec4(color, opacity);
}