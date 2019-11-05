
#version 450

// DECLARATIONS
vec3  getMaterialColor();

// IN
layout(location=0) in vec3 inPosition;        // camera space
// OUT
layout(location=0) out vec4 outPosition;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outDiffuse;
layout(location=3) out vec4 outAmbient;
layout(location=4) out vec4 outSpecular;

void main() {
    outPosition = vec4(inPosition, -1.0);
    outDiffuse = vec4(getMaterialColor(), 1.0);
    outNormal = vec4(0);
    outAmbient = vec4(0);
    outSpecular = vec4(0);
}