
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
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
/**
 * data0[0]: position.x
 * data0[1]: position.y
 * data0[2]: position.z
 * data0[3]: padding
 *
 * data1[0]: veloctiy.x
 * data1[1]: veloctiy.y
 * data1[2]: veloctiy.z
 * data1[3]: age
 *
 * data2[0]: rotation angle
 * data2[1]: rotation velocity
 * data2[2]: padding
 * data2[3]: padding
 */
layout(location=3) in vec4 inData0;
layout(location=4) in vec4 inData1;
layout(location=5) in vec4 inData2;

void main() {

    if (inData1[3] < 0.0) {
        gl_Position = vec4(0);
        return;
    }

    float cs = cos(inData2.x);
    float sn = sin(inData2.x);
    mat4 mRotAndTrans = mat4(
        1, 0, 0, 0,
        0, cs, sn, 0,
        0, -sn, cs, 0,
        inData0.x, inData0.y, inData0.z, 1
    );

    mat4 mMVP = (camera.viewProjection * getModel()) *  mRotAndTrans;
    gl_Position = mMVP * vec4(inPosition, 1.0);
}