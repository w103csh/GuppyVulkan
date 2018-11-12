
#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Camera {
	mat4 mvp;
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
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
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
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main() {
	vec3 worldPosition = vec3(pushConstantsBlock.model * vec4(inPosition, 1.0));
	vec3 worldNormal = normalize(mat3(pushConstantsBlock.model) * inNormal);

	gl_Position = ubo.camera.mvp * vec4(worldPosition, 1.0);
	
	fragPos = worldPosition;
    fragNormal = worldNormal;

    fragTexCoord = inTexCoord;
	fragTexCoord.x *= pushConstantsBlock.material.xRepeat;
	fragTexCoord.y *= pushConstantsBlock.material.yRepeat;
}
