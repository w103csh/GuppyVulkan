
#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Camera {
	mat4 viewProjection;
	mat4 view;
};

struct Material {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    uint shininess;     // Specular shininess factor
	float xRepeat;		// Texture xRepeat
	float yRepeat;		// Texture yRepeat
};

// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
	Material material;
} pushConstantsBlock;
// UNIFORM BUFFER
layout(binding = 0) uniform DefaultUniformBuffer {
	Camera camera;
} ubo;
// OUT
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
	// This obviously can be much more efficient
	mat4 viewModel = ubo.camera.view * pushConstantsBlock.model;
	vec3 cameraSpacePosition = (viewModel * vec4(inPosition, 1.0)).xyz;
	vec3 cameraSpaceNormal = normalize(mat3(viewModel) * inNormal);

	gl_Position = ubo.camera.viewProjection * pushConstantsBlock.model * vec4(inPosition, 1.0);
	
	fragPos = cameraSpacePosition;
    fragNormal = cameraSpaceNormal;
    fragTexCoord = inTexCoord;
	fragTexCoord.x *= pushConstantsBlock.material.xRepeat;
	fragTexCoord.y *= pushConstantsBlock.material.yRepeat;
}
