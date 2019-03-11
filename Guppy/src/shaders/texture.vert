
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define CAM_DEF_PERS 1
#define MAT_DEF 1

// BINDINGS
layout(set = 0, binding = 0) uniform CameraDefaultPerspective {
	mat4 viewProjection;
	mat4 view;
} camera[CAM_DEF_PERS];
layout(set = 0, binding = 1) uniform MaterialDefault {
    vec3 Ka;            // Ambient reflectivity
    uint flags;         // Flags (general/material)
    vec3 Kd;            // Diffuse reflectivity
    float opacity;      // Overall opacity
    vec3 Ks;            // Specular reflectivity
    uint shininess;     // Specular shininess factor
    uint texFlags;      // Texture flags
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
} material;
// } material[MAT_DEF]; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    mat4 model;
} pushConstantsBlock;
// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;
// OUT
layout(location = 0) out vec3 CS_position;
layout(location = 1) out vec3 TS_normal;
layout(location = 2) out vec2 TS_texCoord;
layout(location = 3) out mat3 TBN;

void main() {
	// Camera space transforms
	mat4 viewModel = camera[0].view * pushConstantsBlock.model;
	mat3 viewModel3 = mat3(viewModel);
	vec3 CS_normal = normalize(viewModel3 * inNormal);
	vec3 CS_tangent = normalize(viewModel3 * inTangent);
	vec3 CS_binormal = normalize(viewModel3 * inBinormal);

	gl_Position = camera[0].viewProjection * pushConstantsBlock.model * vec4(inPosition, 1.0);

	// Camera space to tangent space
	// TBN = inverse(mat3(CS_binormal, CS_tangent, CS_normal));
	TBN = transpose(mat3(CS_binormal, CS_tangent, CS_normal));

	CS_position = (viewModel * vec4(inPosition, 1.0)).xyz;
    TS_normal = TBN * CS_normal;
    TS_texCoord = inTexCoord;
	TS_texCoord.x *= material.xRepeat;
	TS_texCoord.y *= material.yRepeat;
}
