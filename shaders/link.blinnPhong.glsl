
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define UMI_LGT_DEF_POS 0
#define UMI_LGT_DEF_SPT 0

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

// FLAGS
// LIGHT
const uint LIGHT_SHOW       = 0x00000001u;
// FOG
const uint FOG_LINEAR       = 0x00000001u;
const uint FOG_EXP          = 0x00000002u;
const uint FOG_EXP2         = 0x00000004u;
const uint _FOG_SHOW        = 0x0000000Fu;

// BINDINGS
layout(set=0, binding=0, std140) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;
layout(set=0, binding=2, std140) uniform UniformDefaultFog {
    float minDistance;
    float maxDistance; 
    float density;
    uint flags;
    // 16
    vec3 color;
    // rem 4
} fog;
#if UMI_LGT_DEF_POS
layout(set=0, binding=3, std140) uniform LightDefaultPositional {
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
layout(set=0, binding=4, std140) uniform LightDefaultSpot {
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
// IN
layout(location=0) in vec3 fragPosition;
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
vec3 toonShade(float sDotN) {
    return Kd * floor(sDotN * ts_levels) * ts_scaleFactor;
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

#if UMI_LGT_DEF_POS
vec3 phongModel(int index, uint lightCount) {
    vec3 ambient = pow(lgtPos[index].La, vec3(lightCount)) * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    // "s" is the direction to the light
    vec3 s = normalize(transform(lgtPos[index].position - fragPosition));
    float sDotN = max(dot(s, n), 0.0);

    // Skip the rest if toon shading
    if (isModeToonShade()) {
        diff = toonShade(sDotN);
        return ambient + lgtPos[index].L * diff;
    }

    // Diffuse color depends on "s" and the "n" only (Lambertian)
    diff = Kd * sDotN;

    spec = vec3(0.0);
    // Only calculate specular if the cosine is positive
    if(sDotN > 0.0) {
        // "h" is the halfway vector between "v" & "s" (Blinn)
        vec3 h = normalize(v + s);
        spec = Ks *
                pow(max(dot(h, n), 0.0), getMaterialShininess());
    }

    return ambient + lgtPos[index].L * (diff + spec);
}
#endif

#if UMI_LGT_DEF_SPT
vec3 blinnPhongSpot(int index, uint lightCount) {
    vec3 ambient = pow(lgtSpot[index].La, vec3(lightCount)) * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    vec3 s = normalize(transform(lgtSpot[index].position - fragPosition));
    float cosAng = dot(-s, normalize(transform(lgtSpot[index].direction)));
    float angle = acos(cosAng);

    float spotScale = 0.0;
    if(angle < lgtSpot[index].cutoff) {
        spotScale = pow(cosAng, lgtSpot[index].exponent);
        float sDotN = max(dot(s,n), 0.0);
        diff = Kd * sDotN;
        if (sDotN > 0.0) {
            vec3 h = normalize(v + s);
            spec = Ks * 
                pow(max(dot(h, n), 0.0), getMaterialShininess());
        }
    }

    return ambient + spotScale * lgtSpot[index].L * (diff + spec);
}
#endif

vec3 blinnPhongShade() {
    vec3 color = vec3(0.0);
    uint lightCount = 0;
    v = normalize(transform(vec3(0.0) - fragPosition));

#if UMI_LGT_DEF_POS
    for (int i = 0; i < lgtPos.length(); i++) {
        if ((lgtPos[i].flags & LIGHT_SHOW) > 0) {
            color += phongModel(i, ++lightCount);
        }
    }
#endif

#if UMI_LGT_DEF_SPT
    for (int i = 0; i < lgtSpot.length(); i++) {
        if ((lgtSpot[i].flags & LIGHT_SHOW) > 0) {
            color += blinnPhongSpot(i, ++lightCount);
        }
    }
#endif

    if ((fog.flags & _FOG_SHOW) > 0) {
        color = mix(fog.color, color, fogFactor());
    }

    return color;
}