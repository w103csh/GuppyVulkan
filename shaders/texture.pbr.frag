
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 pbrShade();
vec3 setTextureDefaults();
vec3 texCoordShade();

// IN
layout(location = 0) in vec3 fragPosition;  // (camera space)
layout(location = 1) in vec3 fragNormal;    // (texture space)
layout(location = 2) in vec2 fragTexCoord;  // (texture space)
layout(location = 3) in mat3 TBN;
// OUT
layout(location = 0) out vec4 outColor;

// GLOBAL
vec3    Kd, // color
        n;  // normal
float opacity;
bool TEX_COORD_SHADE;

/*  TODO: these material getters show that this pattern isn't
    great. Using link shaders like this should just make adding
    new shaders easier. Anything final should be hardcoded
    into one shader file or string in the C++ shader object. */ 
vec3 getMaterialSpecular() { return vec3(1.0); }

vec3 transform(vec3 v) { return TBN * v; }

void main() {
    setTextureDefaults();

    if (TEX_COORD_SHADE) {
        outColor = vec4(texCoordShade(), 1.0);
    } else {
        outColor = vec4(pbrShade(), opacity);
    }
}