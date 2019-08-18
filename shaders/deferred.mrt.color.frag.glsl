    
#version 450

// DECLARATIONS
vec3 setColorDefaults();
float getMaterialShininess();

// IN
layout(location = 0) in vec3 fragPosition;

// OUT
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outDiffuse;
layout (location = 3) out vec4 outAmbient;
layout (location = 4) out vec4 outSpecular;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n;      // normal
float opacity, height;

void main() {
    setColorDefaults();

	outPosition = vec4(fragPosition, 1.0);
    // outNormal = vec4(n, 0.0);
    outNormal = vec4(n, getMaterialShininess());
	outDiffuse = vec4(Kd, opacity);
	outAmbient = vec4(Ka, 0.0);
	outSpecular = vec4(Ks, 0.0);
}