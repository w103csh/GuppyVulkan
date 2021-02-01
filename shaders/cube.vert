/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
#version 450

#define _DS_UNI_DEF 0

// DECLARATIONS
float getMaterialEta();
bool isSkybox();
bool isReflect();
bool isRefract();

// BINDINGS
layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 outWorldPositionReflecionDir;
layout(location=1) out vec3 outRefractDir;

void main() {
    // TODO: move this to separate shader??
    if (isSkybox()) {
        mat4 view = mat4(mat3(camera.view));
        vec4 pos = camera.projection * view * inModel * vec4(inPosition, 1.0);

        outWorldPositionReflecionDir = inPosition;

        gl_Position = pos.xyww;
    } else {
        vec4 worldPos = inModel * vec4(inPosition, 1.0);
        vec3 worldNorm = vec3(inModel * vec4(inNormal, 0.0));
        vec3 worldView = normalize(camera.worldPosition - worldPos.xyz);

        outWorldPositionReflecionDir = reflect(-worldView, normalize(worldNorm));
        if (isRefract())
            outRefractDir = refract(-worldView, normalize(worldNorm), getMaterialEta());

        gl_Position = camera.viewProjection * worldPos;
    }
}
