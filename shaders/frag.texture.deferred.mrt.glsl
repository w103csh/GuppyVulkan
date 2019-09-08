    
#version 450

// DECLARATIONS
vec3 setTextureDefaults();
float getMaterialShininess();

// IN
layout(location=0) in vec3 fragPosition;
layout(location=1) in vec3 fragNormal;
// layout(location=2) in vec2 fragTexCoord;
layout(location=3) in vec3 outTangent;     // (camera space)
layout(location=4) in vec3 outBinormal;    // (camera space)

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

	outPosition = vec4(fragPosition, 1.0);
    // outNormal = vec4(n, 0.0);
    outNormal = vec4(normalize(fragNormal), getMaterialShininess());
	outDiffuse = vec4(Kd, opacity);
	outAmbient = vec4(Ka, 0.0);
	outSpecular = vec4(Ks, 0.0);
}