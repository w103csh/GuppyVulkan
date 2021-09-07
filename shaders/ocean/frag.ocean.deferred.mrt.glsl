/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#version 450

#define _DS_OCEAN 0

// DECLARATIONS
vec3 setColorDefaults();
float getMaterialShininess();
bool isModeFlatShade();

// BINDINGS
layout(set=_DS_OCEAN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

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

const vec3 upwelling = vec3(0.0, 0.2, 0.3);     // TODO: use material Kd?
const vec3 sky = vec3(0.69, 0.84, 1.0);         // TODO: use material Ka? (This could be the clear color potentially)
const vec3 air = vec3(0.1, 0.1, 0.1);           // TODO: use material Ks?
const float nSnell = 1.34;                      // TODO: uniform
const float Kdiffuse = 0.91;                    // TODO: uniform

void main() {
    setColorDefaults();

    outPosition = vec4(inPosition, 1.0);

    outNormal = vec4(n, 0.0);
    outNormal.w = getMaterialShininess();

    vec3 I = normalize(inPosition - camera.worldPosition);

    float reflectivity;
    const vec3 nI = normalize(I);
    const vec3 nN = normalize(n);
    const float costhetai = abs(dot(nI, nN));
    const float thetai = acos(costhetai);
    const float sinthetat = sin(thetai)/nSnell;
    const float thetat = asin(sinthetat);
    if (thetai == 0.0) {
        reflectivity = (nSnell - 1)/(nSnell + 1);
        reflectivity = reflectivity * reflectivity;
    } else {
        float fs = sin(thetat - thetai)
                    / sin(thetat + thetai);
        float ts = tan(thetat - thetai)
                    / tan(thetat + thetai);
        reflectivity = 0.5 * (fs * fs + ts * ts);
    }
    // vec3 dPE = inPosition - camera.worldPosition;
    // float dist = length(dPE) * Kdiffuse * 0.01;
    // dist = exp(-dist);
    const float dist = Kdiffuse;
    const vec3 Ci = dist * (reflectivity * sky + (1.0 - reflectivity) * upwelling)
        + (1.0 - dist) * air;

    outDiffuse = vec4(Ci, opacity);
    outAmbient = vec4(Ci, 0.0);
    outSpecular = vec4(Ci, 0.0);
    outFlags = 0x00u;
}