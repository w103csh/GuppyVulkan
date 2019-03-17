
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define UMI_CAM_DEF_PERS 1

// BINDINGS
layout(binding = 0) uniform CameraDefaultPerspective {
	mat4 viewProjection;
	mat4 view;
} camera[UMI_CAM_DEF_PERS];
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pushConstantsBlock;
// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
// OUT
layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
// layout(location = 3) out int fragVertexIndex;

void main() {
	// This obviously can be much more efficient
	mat4 viewModel = camera[0].view * pushConstantsBlock.model;
	vec3 cameraSpacePosition = (viewModel * vec4(inPosition, 1.0)).xyz;
	vec3 cameraSpaceNormal = normalize(mat3(viewModel) * inNormal);

	gl_Position = camera[0].viewProjection * pushConstantsBlock.model * vec4(inPosition, 1.0);
	// fragVertexIndex = gl_VertexIndex;

	fragPosition = cameraSpacePosition;
    fragNormal = cameraSpaceNormal;
    fragColor = inColor;
}
