
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 blinnPhongShade();
vec3 setTextureDefaults();
vec3 texCoordShade();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// IN
layout(location = 0) in vec3 fragPosition;  // (camera space)
layout(location = 1) in vec3 fragNormal;    // (texture space)
layout(location = 2) in vec2 fragTexCoord;  // (texture space)
// layout(location = 2) centroid in vec2 fragTexCoord;  // (texture space)
layout(location = 3) in mat3 TBN;
// OUT
layout(location = 0) out vec4 outColor;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n;      // normal
float opacity, height;
bool TEX_COORD_SHADE;

vec3 transform(vec3 v) { return TBN * v; }

void main() {
    setTextureDefaults();

    if (TEX_COORD_SHADE) {
        outColor = gammaCorrect(texCoordShade(), 1.0);
    } else {
        // return;
        outColor = gammaCorrect(blinnPhongShade(), opacity);
    }
}