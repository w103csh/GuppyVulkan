
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAT_DEF 1

// DECLARATIONS
vec3 getColor(float shininess);

// APPLICATION CONSTANTS
// TODO: get these from a ubo or something...
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

// MATERIAL FLAGS
const uint PER_MATERIAL_COLOR   = 0x00000001u;
const uint PER_VERTEX_COLOR     = 0x00000002u;
const uint PER_TEXTURE_COLOR    = 0x00000004u;
// TEXTURE FLAGS
const uint TEX_DIFFUSE      = 0x00000001u;
const uint TEX_NORMAL       = 0x00000010u;
const uint TEX_SPECULAR     = 0x00000100u;

// BINDINGS
layout(set = 0, binding = 1) uniform MaterialDefault {
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
} material;
// } material[MAT_DEF];
layout(set = 1, binding = 0) uniform sampler2DArray texSampler;
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pushConstantsBlock;
// IN
layout(location = 0) in vec3 CS_position;
layout(location = 1) in vec3 TS_normal;
layout(location = 2) in vec2 TS_texCoord;
layout(location = 3) in mat3 TBN;
// OUT
layout(location = 0) out vec4 outColor;

bool TEX_COORD_SHADE = false;

vec3 texCoordShade() {
    float modX = floor(mod((TS_texCoord.x * 50), 2.0));
    float modY = floor(mod((TS_texCoord.y * 50), 2.0));
    float total = modX + modY;
    if (total == 1.0) {
        return vec3(1.0);
    } else {
        return vec3(0.45);
    }
}

vec3 getDirToPos(vec3 position) { return normalize(TBN * (position - CS_position)); }
vec3 getDir(vec3 direction) { return normalize(TBN * direction); }

vec3 n, Ka, Kd, Ks;
void main() {
    /*  Sampler offset if based on the Texture::FLAGS enum in C++ and
        the TEX_ constants in GLSL here. The order  in which samplerOffset
        is incremented is important and should match the C++ enum. */
    int samplerOffset = 0;
    float opacity = material.opacity;
    Ka = material.Ka;
    Kd = material.Kd;
    Ks = material.Ks;

    // Diffuse color
    if ((material.texFlags & TEX_DIFFUSE) > 0) {
        vec4 texDiff = texture(texSampler, vec3(TS_texCoord, samplerOffset++));
        Kd = vec3(texDiff);
        opacity = texDiff[3];
        // Use diffuse color for ambient because we haven't created
        // an ambient texture layer yet...
        Ka = Kd;
    } else if ((material.flags & PER_MATERIAL_COLOR) > 0) {
        // Seems superfluous...

    // outColor = vec4(Ka.xyz, 1.0);
    // return;
    } else {
        // This shouldn't happen currently...
    }

    // Normal
    if ((material.texFlags & TEX_NORMAL) > 0) {
        vec3 texNorm = texture(texSampler, vec3(TS_texCoord, samplerOffset++)).xyz;
        n = 2.0 * texNorm - 1.0;
    } else {
	    n = TS_normal;
    }

    // Specular color
    Ks = material.Ks;
    if ((material.texFlags & TEX_SPECULAR) > 0) {
        vec4 texSpec = texture(texSampler, vec3(TS_texCoord, samplerOffset++));
        Ks = vec3(texSpec);
    }

    if (TEX_COORD_SHADE) {
        outColor = vec4(texCoordShade(), 1.0);
        return;
    }

    outColor = vec4(
        getColor(material.shininess),
        material.opacity
    );
}