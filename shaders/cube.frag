
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DECLARATIONS
vec3 getMaterialColor();
float getMaterialOpacity();
float getMaterialReflectionFactor();
bool isSkybox();
bool isReflect();
bool isRefract();

layout(set=1, binding=0) uniform samplerCube sampCube;
// IN
layout(location=0) in vec3 fragWorldPositionReflecionDir;   // (world space)
layout(location=1) in vec3 fragRefractDir;                  // (world space)
// OUT
layout(location=0) out vec4 outColor;

// GLOBAL
vec3    Kd;

void main() {
    vec4 sampCubeColor = texture(sampCube, normalize(fragWorldPositionReflecionDir));
    if (isSkybox()) {
        outColor = sampCubeColor;
    } else  {
        vec3 baseColor;
        if (isReflect())
            baseColor = getMaterialColor();
        if (isRefract())
            baseColor = texture(sampCube,  normalize(fragRefractDir)).rgb;

        outColor = vec4(
            mix(baseColor, sampCubeColor.rgb, getMaterialReflectionFactor()),
            getMaterialOpacity()
        );

    }
    // outColor = pow(outColor, vec4(1.0/2.2));
}