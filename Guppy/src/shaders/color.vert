
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec4 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    fragNormal = inNormal;
    fragColor = inColor;
}
