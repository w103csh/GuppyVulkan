
#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Camera {
	mat4 viewProjection;
	mat4 view;
};

// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pushConstantsBlock;
// UNIFORM BUFFER
layout(binding = 0) uniform DefaultUniformBuffer {
	Camera camera;
} ubo;
// OUT
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
// layout(location = 3) out int fragVertexIndex;

void main() {
	// This obviously can be much more efficient
	mat4 viewModel = ubo.camera.view * pushConstantsBlock.model;
	vec3 cameraSpacePosition = (viewModel * vec4(inPosition, 1.0)).xyz;
	vec3 cameraSpaceNormal = normalize(mat3(viewModel) * inNormal);

	gl_Position = ubo.camera.viewProjection * pushConstantsBlock.model * vec4(inPosition, 1.0);
	// fragVertexIndex = gl_VertexIndex;

	fragPos = cameraSpacePosition;
    fragNormal = cameraSpaceNormal;
    fragColor = inColor;
}
