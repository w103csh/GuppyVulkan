
#version 450
#extension GL_ARB_separate_shader_objects : enable

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;
// OUT
layout(location=0) out vec2 fragTexCoord;

void main() {
    fragTexCoord = inTexCoord;
    gl_Position.x =  inPosition.x;
    gl_Position.y = -inPosition.y; // Vulkan y is inverted
    gl_Position.z =  inPosition.z;
    gl_Position.w =  1.0;
}