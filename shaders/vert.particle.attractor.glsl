
#version 450

#define _DS_UNI_PRTCL_FNTN 0

// DECLARATIONS
mat4  getModel();

// BINDINGS
layout(set=_DS_UNI_PRTCL_FNTN, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

// IN
/**
 * data[0]: position.x
 * data[1]: position.y
 * data[2]: position.z
 * data[3]: padding
 */
layout(location=0) in vec4 inData;

// OUT
layout(location=0) out vec3 outPosition;        // camera space

void main() {
    outPosition = ((camera.view * getModel()) * vec4(inData.xyz, 1.0)).xyz;
    gl_Position = camera.projection * vec4(outPosition, 1.0);
    gl_PointSize = 1.0;
}