
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define UMI_LGT_DEF_POS 0
#define UMI_LGT_DEF_SPT 0
#define DSMI_UNI_DEF 0
#define DSMI_SMP_DEF 0
#define DSMI_PRJ_DEF -1

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
layout(set=DSMI_UNI_DEF, binding=0, std140) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

layout(set=DSMI_UNI_DEF, binding=2, std140) uniform UniformDefaultFog {
    float minDistance;
    float maxDistance; 
    float density;
    uint flags;
    // 16
    vec3 color;
    // rem 4
} fog;

#if UMI_LGT_DEF_POS
layout(set=DSMI_UNI_DEF, binding=3) uniform LightDefaultPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    // 16
    vec3 La;        // Ambient light intesity
    // rem 4
    vec3 L;         // Diffuse and specular light intensity
    // rem 4
} lgtPos[UMI_LGT_DEF_POS];
#endif

#if UMI_LGT_DEF_SPT
layout(set=DSMI_UNI_DEF, binding=4) uniform LightDefaultSpot {
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
} lgtSpot[UMI_LGT_DEF_SPT];
#endif

#if DSMI_PRJ_DEF > -1
layout(set=DSMI_PRJ_DEF, binding=0) uniform sampler2D sampProjector;
#endif

// IN
layout(location=0) in vec3 fragPosition;
#if DSMI_PRJ_DEF > -1
layout(location=6) in vec4 fragProjTexCoord;
layout(location=7) in vec4 fragTest;
#endif
// OUT
layout(location=0) out vec4 outColor;

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
    // float dist = abs(fragPosition.z); // faster version
    float z = length(fragPosition.xyz);

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
#if DSMI_PRJ_DEF >= -1
    if(fragProjTexCoord.z > 0.0) {
        vec3 projColor = textureProj(sampProjector, fragProjTexCoord).rgb;
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
    v = normalize(transform(vec3(0.0) - fragPosition));
    float shininess = getMaterialShininess();

#if UMI_LGT_DEF_POS
    for (int i = 0; i < lgtPos.length(); i++) {
        if ((lgtPos[i].flags & LIGHT_SHOW) > 0) {

            if (isModeToonShade()) {

                color += toonShade(
                    pow(lgtPos[i].La, vec3(++lightCount)),
                    lgtPos[i].L,
                    normalize(transform(lgtPos[i].position - fragPosition))
                );

            } else {

                color += blinnPhongPositional(
                    pow(lgtPos[i].La, vec3(++lightCount)),
                    lgtPos[i].L,
                    normalize(transform(lgtPos[i].position - fragPosition)),
                    shininess
                );

            }

        }
    }
#endif

#if UMI_LGT_DEF_SPT
    for (int i = 0; i < lgtSpot.length(); i++) {
        if ((lgtSpot[i].flags & LIGHT_SHOW) > 0) {

            color += blinnPhongSpot(
                pow(lgtSpot[i].La, vec3(++lightCount)),
                lgtSpot[i].L,
                normalize(transform(lgtSpot[i].position - fragPosition)),
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