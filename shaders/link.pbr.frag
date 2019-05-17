
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define UMI_LGT_PBR_POS 0

// DECLARATIONS
vec3 transform(vec3 v);

// APPLICATION CONSTANTS (TODO: get these from a ubo or something)
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space
const float PI = 3.14159265358979323846;

// MATERIAL FLAGS
const uint METAL   = 0x00001000u;

// BINDINGS
layout(set = 0, binding = 1, std140) uniform MaterialPBR {
    vec3 color;         // Diffuse color for dielectrics, f0 for metallic
    float opacity;      // Overall opacity
    // 16
    uint flags;         // Flags (general/material)
    uint texFlags;      // Flags (texture)
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
    // 16
    float rough;        // Roughness
    // rem 12
} material;
#if UMI_LGT_PBR_POS
layout(set = 0, binding = 2, std140) uniform LightPBRPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    vec3 L;         // Diffuse and specular light intensity
    // rem 4
} lgtPos[UMI_LGT_PBR_POS];
#endif

// IN
layout(location = 0) in vec3 fragPosition;  // (camera space)
layout(location = 1) in vec3 fragNormal;    // (texture space)
// OUT
layout(location = 0) out vec4 outColor;

// GLOBAL
vec3 Kd, n;

vec3 getMaterialColor() { return material.color; }
uint getMaterialFlags() { return material.flags; }
uint getMaterialTexFlags() { return material.texFlags; }
float getMaterialOpacity() { return material.opacity; }
float getMaterialXRepeat() { return material.xRepeat; }
float getMaterialYRepeat() { return material.yRepeat; }

vec3 schlickFresnel(float lDotH) {
    vec3 f0 = vec3(0.0f); // Dielectrics
    if ((material.flags & METAL) > 0) {
        f0 = Kd;
    }
    return f0 + (1 - f0) * pow(1.0 - lDotH, 5);
}

float geomSmith(float dotProduct) {
    float k = (material.rough + 1.0) * (material.rough + 1.0) / 8.0;
    float denom = dotProduct * (1 - k) + k;
    return 1.0 / denom;
}

float ggxDistribution(float nDotH) {
    float alpha2 = material.rough * material.rough * material.rough * material.rough;
    float d = (nDotH * nDotH) * (alpha2 - 1) + 1;
    return alpha2 / (PI * d * d);
}

vec3 microfacetModel(int index) {
    vec3 diffuseBrdf = vec3(0.0);  // Metallic
    if((material.flags & METAL) == 0) {
        diffuseBrdf = Kd;
    }

    vec3 l = vec3(0.0), L = lgtPos[index].L;

     // Directional light
    // if(light.position.w == 0.0) {
    //     l = normalize(light.position.xyz);
    // // Positional light
    // } else {
        l = transform(lgtPos[index].position - fragPosition); // direction to light
        float dist = length(l);
        l = normalize(l);
        L /= (dist * dist);
    // }

    // return n;
    // return l;
    // return vec3(lgtPos[lgtIndex].position.x, lgtPos[lgtIndex].position.y, lgtPos[lgtIndex].position.z);
    // return vec3(1.0) / L;

    vec3 v = normalize(transform(vec3(0.0) - fragPosition)); // direction to camera
    vec3 h = normalize(v + l); // halfway vector
    float nDotH = dot(n, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = dot(n, v);

    // return v;
    // return h;
    // return vec3(nDotH);
    // return vec3(lDotH);
    // return vec3(nDotL);
    // return vec3(nDotV);

    // return Kd;

    // return schlickFresnel(lDotH)
    //     * ggxDistribution(nDotH)
    //     * geomSmith(nDotL)
    //     * geomSmith(nDotV)
    //     ;

    vec3 specBrdf = 0.25 * ggxDistribution(nDotH) * schlickFresnel(lDotH) * geomSmith(nDotL) * geomSmith(nDotV);

    // return diffuseBrdf;
    // return specBrdf;

    return (diffuseBrdf + PI * specBrdf) * L * nDotL;
}

bool epsilonEqual(vec3 v1, vec3 v2) {
    float eps = 0.000001;
    bvec3 comp = bvec3(
        (abs(v1.x - v2.x) < eps),
        (abs(v1.y - v2.y) < eps),
        (abs(v1.z - v2.z) < eps)
    );
    return all(comp);
}

vec3 pbrShade() {
    vec3 sum = vec3(0.0);
#if UMI_LGT_PBR_POS
    for (int i = 0; i < lgtPos.length(); i++) {
        // if (i == 0) continue;
        sum += microfacetModel(i);
    }
#endif
    return pow(sum, vec3(1.0/screenGamma));
}