
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_POS_LIGHTS 0
#define HAS_POS_LIGHTS NUM_POS_LIGHTS > 0
#define NUM_SPOT_LIGHTS 0
#define HAS_SPOT_LIGHTS NUM_SPOT_LIGHTS > 0

// DECLARATIONS
// Gets the direction from the fragment to the position.
vec3 getDirToPos(vec3 position);
// Transform and normalize a direction to local coordinate space.
vec3 getDir(vec3 direction);

#if HAS_POS_LIGHTS || HAS_SPOT_LIGHTS
// LIGHT flags
const uint LIGHT_SHOW    = 0x00000001u;
#endif

#if HAS_POS_LIGHTS
struct PositionalLight {
    vec3 position;  // Light position in eye coords.
    uint flags;
    vec3 La;        // Ambient light intesity
    vec3 L;         // Diffuse and specular light intensity
};
#endif
#if HAS_SPOT_LIGHTS
struct SpotLight {
    vec3 position;
    uint flags;
    vec3 La;
    float exponent;
    vec3 L;
    float cutoff;
    vec3 direction;
};
#endif

// SHADER flags
const uint SHADER_DEFAULT       = 0x00000000u;
const uint SHADER_TOON_SHADE    = 0x00000001u;
// SHADER variables
const int ts_levels = 3;
const float ts_scaleFactor = 1.0 / ts_levels;

struct Camera {
	mat4 viewProjection;
	mat4 view;
};

// UNIFORM buffer
layout(binding = 0) uniform DefaultUniformBuffer {
	Camera camera;
    uint shaderFlags; // 12 rem
#if HAS_POS_LIGHTS
	PositionalLight positionLights[NUM_POS_LIGHTS];
#endif
#if HAS_SPOT_LIGHTS
    SpotLight spotLights[NUM_SPOT_LIGHTS];
#endif
} ubo;

vec3 n, Ka, Kd, Ks;

vec3 toonShade(float sDotN) {
    return Kd * floor(sDotN * ts_levels) * ts_scaleFactor;
}

#if HAS_POS_LIGHTS
vec3 phongModel(PositionalLight light, float shininess, uint lightCount) {
    vec3 ambient = pow(light.La, vec3(lightCount)) * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    // "s" is the direction to the light
    vec3 s = getDirToPos(light.position);
    float sDotN = max(dot(s, n), 0.0);

    // Skip the rest if toon shading
    if ((ubo.shaderFlags & SHADER_TOON_SHADE) > 0) {
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

#if HAS_SPOT_LIGHTS
vec3 blinnPhongSpot(SpotLight light, float shininess, uint lightCount) {
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

#if HAS_POS_LIGHTS
    for (int i = 0; i < ubo.positionLights.length(); i++) {
        if ((ubo.positionLights[i].flags & LIGHT_SHOW) > 0) {
            color += phongModel(ubo.positionLights[i], shininess, ++lightCount);
        }
    }
#endif

#if HAS_SPOT_LIGHTS
    for (int i = 0; i < ubo.spotLights.length(); i++) {
        if ((ubo.spotLights[i].flags & LIGHT_SHOW) > 0) {
            color += blinnPhongSpot(ubo.spotLights[i], shininess, ++lightCount);
        }
    }
#endif

    return color;
}