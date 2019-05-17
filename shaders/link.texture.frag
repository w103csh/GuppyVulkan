
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
vec3 transform(vec3 v);

// TEXTURE FLAGS
//  TYPE MASK
const uint TEX_COLOR            = 0x0000000Fu; // diffuse
const uint TEX_NORMAL           = 0x000000F0u;
const uint TEX_SPECULAR         = 0x00000F00u;
const uint TEX_ALPHA            = 0x0000F000u;
const uint TEX_HEIGHT           = 0x000F0000u;
//  CHANNELS
const uint TEX_CH_1             = 0x01u;
const uint TEX_CH_2             = 0x02u;
const uint TEX_CH_3             = 0x04u;
const uint TEX_CH_4             = 0x08u;

layout(set = 1, binding = 0) uniform sampler2DArray sampCh4;
// layout(set = 1, binding = 1) uniform sampler2DArray sampCh1;
// layout(set = 1, binding = 2) uniform sampler2DArray sampCh2;
// layout(set = 1, binding = 3) uniform sampler2DArray sampCh3;
// IN
layout(location = 0) in vec3 fragPosition;  // (texture space)
layout(location = 1) in vec3 fragNormal;    // (texture space)
layout(location = 2) in vec2 fragTexCoord;  // (texture space)
// OUT
layout(location = 0) out vec4 outColor;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n,      // normal
        v;      // direction to the camera
float opacity, height;
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

// This is obviously not fast, but like the rest of the shaders so far it
// is designed for utility.
int sampCh1Cnt, sampCh2Cnt, sampCh3Cnt, sampCh4Cnt;
bool getSample(const in vec2 texCoord, const in uint flags, uint typeMask, out vec4 samp) {
    if (flags == 0) return false;
    uint channelMask = flags & typeMask;
    if (channelMask > 0) {
        while ((channelMask & 0x0Fu) == 0) channelMask >>= 4;
        switch(channelMask) {
            // case 0x01u: samp = texture(sampCh1, vec3(texCoord, sampCh1Cnt++)); break;
            // case 0x02u: samp = texture(sampCh2, vec3(texCoord, sampCh2Cnt++)); break;
            // case 0x04u: samp = texture(sampCh3, vec3(texCoord, sampCh3Cnt++)); break;
            case 0x08u: samp = texture(sampCh4, vec3(texCoord, sampCh4Cnt++)); break;
        }
        return true;
    }
    return false;
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
    vec4 samp;
    sampCh1Cnt = sampCh2Cnt = sampCh3Cnt = sampCh4Cnt = 0;
    vec2 texCoord = vec2((fragTexCoord.x * xRepeat), (fragTexCoord.y * yRepeat));

    // COLOR (diffuse)
    if (getSample(texCoord, flags, TEX_COLOR, samp)) {
        Kd = samp.xyz;
        opacity = samp[3];
        // Use diffuse color for ambient because we haven't created
        // an ambient texture layer yet...
        Ka = Kd;
    }

    // NORMAL
    if (getSample(texCoord, flags, TEX_NORMAL, samp)) {
        n = samp.xyz;
        n = 2.0 * n - 1.0;
    } else {
	    n = fragNormal;
    }

    // SPECULAR
    if (getSample(texCoord, flags, TEX_SPECULAR, samp)) {
        Ks = samp.xyz;
    }

    //  ALPHA
    if (getSample(texCoord, flags, TEX_ALPHA, samp)) {
        opacity = samp.x;
        if (opacity < 0.7)
            discard;
    }

    /*  It looks to me like if the texture is defining the outline of an object
        with opacity then as many fragments at the edge need to be discarded as possible.

        THERE MIGHT BE A WAY TO CLEAN THIS UP ELSEWHERE IN VkPipelineColorBlendAttachmentState
        OR SOME OTHER SETTING!!!
        
        If there isn't then there should probably be shaders dedicated to dropping
        the transparent fragments aggressively, and one dedicated to blending
        them properly, OR A TEXTURE FLAG!

        From OpenGL4 Shading Language Cookbook p.153:
            "... However, That requires us to make the depth buffer read-only and render
            all of out polygons from back to front in order to avoid blending problems.
            We would need to sort our polygons from based on the camera position and 
            then render them in the correct order."
        They then recommend using a value 0.15 for discard so, I guess my approach
        wasn't too far fetched.
        
        I stored the alpha map value in the diffuse texture's 4th channel, so we look for
        it here (but I could change that if it produces better results).
    */
    if (opacity < 0.15)
        discard;
}