
#version 450
#extension GL_ARB_separate_shader_objects : enable

// BINDINGS
layout(binding = 0) uniform CameraDefaultPerspective {
	mat4 viewProjection;
	mat4 view;
} camera;
// // PUSH CONSTANTS
// layout(push_constant) uniform PushBlock {
//     mat4 model;
// } pushConstantsBlock;
// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in mat4 inModel;
// OUT
layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
// layout(location = 3) out int fragVertexIndex;

void main() {
	// This obviously can be much more efficient
	mat4 viewModel = camera.view * inModel;
	vec3 cameraSpacePosition = (viewModel * vec4(inPosition, 1.0)).xyz;
	vec3 cameraSpaceNormal = normalize(mat3(viewModel) * inNormal);

	gl_Position = camera.viewProjection * inModel * vec4(inPosition, 1.0);
	// fragVertexIndex = gl_VertexIndex;

	fragPosition = cameraSpacePosition;
    fragNormal = cameraSpaceNormal;
    fragColor = inColor;
}
