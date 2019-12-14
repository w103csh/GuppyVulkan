/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DEF 0
#define _DS_SMP_DEF 0

// DECLARATIONS
vec3 getMaterialColor();
float getMaterialEta();
float getMaterialOpacity();
float getMaterialReflectionFactor();
bool isModeFlatShade();
bool isSkybox();
bool isReflect();
bool isRefract();
vec4 gammaCorrect(const in vec3 color, const in float opacity);

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

layout(set=_DS_SMP_DEF, binding=0) uniform samplerCube sampCube;

// IN
layout(location=0) in vec3 inPosition;    // (camera space)
layout(location=1) in vec3 inNormal;      // (camera space)
// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;
layout(location=5) out uint outFlags;

void main() {
    vec3 I = normalize(inPosition - camera.worldPosition);
    vec3 reflectDir = reflect(I, normalize(inNormal));
    vec3 sampCubeColor = texture(sampCube, reflectDir).rgb;

    vec3 baseColor = getMaterialColor();
    if (isRefract()) {
        vec3 refractDir = refract(I, normalize(inNormal), getMaterialEta());
        if (all(equal(refractDir, vec3(0.0))))
            refractDir = reflectDir;
        baseColor = texture(sampCube, refractDir).rgb;
    }

    outDiffuse = gammaCorrect(
        mix(baseColor, sampCubeColor, getMaterialReflectionFactor()),
        getMaterialOpacity()
    );
    outAmbient = outDiffuse;
    outSpecular = outDiffuse;

    outPosition = camera.view * vec4(inPosition, 1.0);
    outNormal = vec4(mat3(camera.view) * inNormal, 0.0);

    outFlags = isModeFlatShade() ? 0x01 : 0x00;
}