
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 getMaterialColor();
vec3 getMaterialSpecular();
uint getMaterialTexFlags();
float getMaterialOpacity();
float getMaterialShininess();
float getMaterialXRepeat();
float getMaterialYRepeat();

// TEXTURE FLAGS
const uint TEX_DIFFUSE      = 0x00000001u;
const uint TEX_NORMAL       = 0x00000010u;
const uint TEX_SPECULAR     = 0x00000100u;

layout(set = 1, binding = 0) uniform sampler2DArray texSampler;
// IN
layout(location = 1) in vec3 fragNormal;    // (texture space)
layout(location = 2) in vec2 fragTexCoord;  // (texture space)
// OUT
layout(location = 0) out vec4 outColor;

// GLOBAL
vec3    Ka, // ambient coefficient
        Kd, // diffuse coefficient
        Ks, // specular coefficient
        n;  // normal
float opacity;
bool TEX_COORD_SHADE = false;

vec3 texCoordShade() {
    float modX = floor(mod((fragTexCoord.x * 50), 2.0));
    float modY = floor(mod((fragTexCoord.y * 50), 2.0));
    float total = modX + modY;
    if (total == 1.0) {
        return vec3(1.0);
    } else {
        return vec3(0.45);
    }
}

void setTextureDefaults() {
    Kd = getMaterialColor();
    Ks = getMaterialSpecular();
    opacity = getMaterialOpacity();
    uint flags = getMaterialTexFlags();
    float xRepeat = getMaterialXRepeat();
    float yRepeat = getMaterialYRepeat();

    /*  Sampler offset if based on the Texture::FLAGS enum in C++ and
        the TEX_ constants in GLSL here. The order  in which samplerOffset
        is incremented is important and should match the C++ enum. */
    int samplerOffset = 0;
    vec2 texCoord = vec2((fragTexCoord.x * xRepeat), (fragTexCoord.y * yRepeat));

    // Diffuse color
    if ((flags & TEX_DIFFUSE) > 0) {
        vec4 texDiff = texture(texSampler, vec3(texCoord, samplerOffset++));
        Kd = vec3(texDiff);
        opacity = texDiff[3];
        // Use diffuse color for ambient because we haven't created
        // an ambient texture layer yet...
        Ka = Kd;
    }

    // Normal
    if ((flags & TEX_NORMAL) > 0) {
        vec3 normal = texture(texSampler, vec3(texCoord, samplerOffset++)).xyz;
        n = 2.0 * normal - 1.0;
    } else {
	    n = fragNormal;
    }

    // Specular color
    if ((flags & TEX_SPECULAR) > 0) {
        vec4 spec = texture(texSampler, vec3(texCoord, samplerOffset++));
        Ks = vec3(spec);
    }
}