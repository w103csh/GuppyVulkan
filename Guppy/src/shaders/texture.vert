
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
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;
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
layout(location = 0) out vec3 CS_position;
layout(location = 1) out vec3 TS_normal;
layout(location = 2) out vec2 TS_texCoord;
layout(location = 3) out mat3 TBN;

void main() {
	// Camera space transforms
	mat4 viewModel = ubo.camera.view * pushConstantsBlock.model;
	vec3 CS_normal = normalize(mat3(viewModel) * inNormal);
	vec3 CS_tangent = normalize(mat3(viewModel) * inTangent);
	vec3 CS_binormal = normalize(mat3(viewModel) * inBinormal);

	gl_Position = ubo.camera.viewProjection * pushConstantsBlock.model * vec4(inPosition, 1.0);

	// camera space to tangent space
	TBN = transpose(mat3(CS_tangent, CS_binormal, CS_normal));

	CS_position = (viewModel * vec4(inPosition, 1.0)).xyz;
    TS_normal = TBN * CS_normal;
    TS_texCoord = inTexCoord;
	TS_texCoord.x *= pushConstantsBlock.material.xRepeat;
	TS_texCoord.y *= pushConstantsBlock.material.yRepeat;
}
