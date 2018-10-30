
#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Camera {
	mat4 mvp;
	vec3 position;
};

layout(binding = 0) uniform DefaultUBO {
	Camera camera;
} ubo;

// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;

// OUT
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec3 fragNormal;
out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	gl_Position = ubo.camera.mvp * vec4(inPosition, 1.0);
    fragPos = inPosition;
    fragColor = inColor;
    fragNormal = inNormal;
}
