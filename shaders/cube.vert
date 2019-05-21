
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
float getMaterialEta();
bool isSkybox();
bool isReflect();
bool isRefract();

// BINDINGS
layout(set=0, binding=0) uniform CameraDefaultPerspective {
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
layout(location=0) out vec3 fragWorldPositionReflecionDir;
layout(location=1) out vec3 fragRefractDir;

void main() {
    // TODO: move this to separate shader??
    if (isSkybox()) {
        mat4 view = mat4(mat3(camera.view));
        vec4 pos = camera.projection * view * inModel * vec4(inPosition, 1.0);

        fragWorldPositionReflecionDir = inPosition;

        gl_Position = pos.xyww;
    } else {
        vec4 worldPos = inModel * vec4(inPosition, 1.0);
        vec3 worldNorm = vec3(inModel * vec4(inNormal, 0.0));
        vec3 worldView = normalize(camera.worldPosition - worldPos.xyz);

        fragWorldPositionReflecionDir = reflect(-worldView, normalize(worldNorm));
        if (isRefract())
            fragRefractDir = refract(-worldView, normalize(worldNorm), getMaterialEta());

        gl_Position = camera.viewProjection * worldPos;
    }
}
