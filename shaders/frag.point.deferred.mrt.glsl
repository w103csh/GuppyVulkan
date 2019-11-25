
#version 450

// DECLARATIONS
vec3  getMaterialColor();
bool isModeFlatShade();

// IN
layout(location=0) in vec3 inPosition;        // (camera space)
layout(location=1) in flat uint inFlags;
// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;
layout(location=5) out uint outFlags;

void main() {
    outPosition = vec4(inPosition, -1.0);
    outDiffuse = vec4(getMaterialColor(), 1.0);
    outNormal = vec4(0);
    outAmbient = vec4(0);
    outSpecular = vec4(0);
    outFlags = inFlags | (isModeFlatShade() ? 0x1u : 0x0u);
}