
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define CAM_DEF_PERS 1
#define MAT_DEF 1
#define LGT_DEF_POS 0
#define LGT_DEF_SPT 0
#define UNI_DEF_FOG 1

// DECLARATIONS
// Gets the direction from the fragment to the position.
vec3 getDirToPos(vec3 position);
// Transform and normalize a direction to local coordinate space.
vec3 getDir(vec3 direction);

// FLAGS 
// LIGHT
const uint LIGHT_SHOW       = 0x00000001u;
// MATERIAL
const uint MODE_TOON_SHADE  = 0x00000040u;
// FOG
const uint FOG_LINEAR       = 0x00000001u;
const uint FOG_EXP          = 0x00000002u;
const uint FOG_EXP2         = 0x00000004u;
const uint _FOG_SHOW        = 0x0000000Fu;

// SHADER variables
const int ts_levels = 3;
const float ts_scaleFactor = 1.0 / ts_levels;

struct CameraDefaultPerspective {
	mat4 viewProjection;
	mat4 view;
};
struct MaterialDefault {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    float shininess;    // Specular shininess factor
    uint texFlags;      // Texture flags
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
    float _padding;
};
struct UniformDefaultFog {
    float minDistance;
    float maxDistance; 
    float density;
    uint flags;
    vec3 color; // rem 4
};
struct LightDefaultPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    vec3 La;        // Ambient light intesity
    vec3 L;         // Diffuse and specular light intensity
};
struct LightDefaultSpot {
    vec3 position;
    uint flags;
    vec3 La;
    float exponent;
    vec3 L;
    float cutoff;
    vec3 direction;
    uint _padding;
};

// BINDINGS
layout(set = 0, binding = 0) uniform Binding0 {
    CameraDefaultPerspective camera;
} binding0[CAM_DEF_PERS];
layout(set = 0, binding = 1) uniform Binding1 {
    MaterialDefault material;
} binding1[MAT_DEF];
layout(set = 0, binding = 2) uniform Binding2 {
    UniformDefaultFog fog;
} binding2[UNI_DEF_FOG];
#if LGT_DEF_POS
layout(set = 0, binding = 3) uniform Binding3 {
    LightDefaultPositional lgtPos;
} binding3[LGT_DEF_POS];
#endif
#if LGT_DEF_SPT
layout(set = 0, binding = 4) uniform Binding4 {
    LightDefaultSpot lgtSpt;
} binding4[LGT_DEF_SPT];
#endif
// IN
layout(location = 0) in vec3 CS_position;
layout(location = 1) in vec3 TS_normal;

vec3 n, Ka, Kd, Ks;

vec3 toonShade(float sDotN) {
    return Kd * floor(sDotN * ts_levels) * ts_scaleFactor;
}

float fogFactor() {
    float f = 0.0f;
    // float dist = abs(CS_position.z); // faster version
    float z = length(CS_position.xyz);

    // Each of below has slightly different properties. Read
    // the book again if you want to hear their explanation.
    if ((binding2[0].fog.flags & FOG_LINEAR) > 0) {
        f = (binding2[0].fog.maxDistance - z) / (binding2[0].fog.maxDistance - binding2[0].fog.minDistance);
    }
    else if ((binding2[0].fog.flags & FOG_EXP) > 0) {
        f = exp(-binding2[0].fog.density * z);
    }
    else if ((binding2[0].fog.flags & FOG_EXP2) > 0) {
        f = exp(-pow((binding2[0].fog.density * z), 2));
    }
    return clamp(f, 0.0, 1.0);
}

#if LGT_DEF_POS
vec3 phongModel(LightDefaultPositional light, float shininess, uint lightCount) {
    vec3 ambient = pow(light.La, vec3(lightCount)) * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    // "s" is the direction to the light
    vec3 s = getDirToPos(light.position);
    float sDotN = max(dot(s, n), 0.0);

    // return Kd;

    // Skip the rest if toon shading
    if ((binding1[0].material.flags & MODE_TOON_SHADE) > 0) {
        diff = toonShade(sDotN);
        return ambient + light.L * diff;
    }

    // Diffuse color depends on "s" and the "n" only (Lambertian)
    diff = Kd * sDotN;

    spec = vec3(0.0);
    // Only calculate specular if the cosine is positive
    if(sDotN > 0.0) {
        // "v" is the direction to the camera
        vec3 v = getDirToPos(vec3(0.0));
        // "h" is the halfway vector between "v" & "s" (Blinn)
        vec3 h = normalize(v + s);

        spec = Ks *
                pow(max(dot(h, n), 0.0), shininess);
    }

    return ambient + light.L * (diff + spec);
}
#endif

#if LGT_DEF_SPT
vec3 blinnPhongSpot(LightDefaultSpot light, float shininess, uint lightCount) {
    vec3 ambient = pow(light.La, vec3(lightCount)) * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    vec3 s = getDirToPos(light.position);
    float cosAng = dot(-s, getDir(light.direction));
    float angle = acos(cosAng);

    float spotScale = 0.0;
    if(angle < light.cutoff) {
        spotScale = pow(cosAng, light.exponent);
        float sDotN = max(dot(s,n), 0.0);
        diff = Kd * sDotN;
        if (sDotN > 0.0) {
            vec3 v = getDirToPos(vec3(0.0));
            vec3 h = normalize(v + s);
            spec = Ks * 
                pow(max(dot(h, n), 0.0), shininess);
        }
    }

    return ambient + spotScale * light.L * (diff + spec);
}
#endif

vec3 getColor(float shininess) {
    vec3 color = vec3(0.0);
    uint lightCount = 0;

#if LGT_DEF_POS
    for (int i = 0; i < binding3.length(); i++) {
        if ((binding3[i].lgtPos.flags & LIGHT_SHOW) > 0) {
            color += phongModel(binding3[i].lgtPos, shininess, ++lightCount);
        }
    }
#endif

#if LGT_DEF_SPT
    for (int i = 0; i < binding4.length(); i++) {
        if ((binding4[i].lgtSpt.flags & LIGHT_SHOW) > 0) {
            color += blinnPhongSpot(binding4[i].lgtSpt, shininess, ++lightCount);
        }
    }
#endif

    if ((binding2[0].fog.flags & _FOG_SHOW) > 0) {
        color = mix(binding2[0].fog.color, color, fogFactor());
    }

    return color;
}