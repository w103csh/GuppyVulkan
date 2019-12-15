/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
     
#version 450

// DECLARATIONS
vec3 setColorDefaults();
float getMaterialShininess();
bool isModeFlatShade();
void geomShade(inout vec4 color);

// IN
layout(location=0) in vec3 inPosition;

// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;
layout(location=5) out uint outFlags;

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n;      // normal
float opacity, height;

void main() {
    setColorDefaults();

	outPosition = vec4(inPosition, 1.0);
    
    outNormal = vec4(n, 0.0);
    if (!gl_FrontFacing) {
        outNormal *= -1.0;
        // outTangent *= -1.0;
        // outBinormal *= -1.0;
    }
    outNormal.w = getMaterialShininess();
	outDiffuse = vec4(Kd, opacity);
	outAmbient = vec4(Ka, 0.0);
	outSpecular = vec4(Ks, 0.0);
    outFlags = isModeFlatShade() ? 0x01u : 0x00u;

    geomShade(outDiffuse);
}