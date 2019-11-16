    
#version 450

// DECLARATIONS
vec3 setTextureDefaults();
float getMaterialShininess();

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
// layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;     // (camera space)
layout(location=4) in vec3 inBinormal;    // (camera space)

// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n;      // normal
float opacity, height;

void main() {
    setTextureDefaults();

	outPosition = vec4(inPosition, 1.0);
    
    outNormal = vec4(normalize(inNormal), 0.0);
    if (!gl_FrontFacing) {
        outNormal *= -1.0;
        // outTangent *= -1.0;
        // outBinormal *= -1.0;
    }
    outNormal.w = getMaterialShininess();
	outDiffuse = vec4(Kd, opacity);
	outAmbient = vec4(Ka, 0.0);
	outSpecular = vec4(Ks, 0.0);
}