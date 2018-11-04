
#version 450
#extension GL_ARB_separate_shader_objects : enable

// LIGHT FLAGS
const uint DL_HIDE          = 0x00000001u;
const uint DL_SHOW          = 0x00000002u;

struct Camera {
	mat4 mvp;
	vec3 position;
};

struct PositionalLight {
    vec4 position;  // Light position in eye coords.
    vec3 La;        // Ambient light intesity
    uint flags;     // flags
    vec3 L;         // Diffuse and specular light intensity
};

// IN
layout(location = 0) in vec3 fragPos;
// UNIFORM BUFFER
layout(binding = 0) uniform DefaultUniformBuffer {
	Camera camera;
	PositionalLight light;
} ubo;

vec3 getColor(vec3 n, vec3 ka, vec3 kd, vec3 ks, uint shininess) {
    vec3 diff, spec;
    diff = spec = vec3(0.0);
    vec3 ambient = ubo.light.La * ka;

    if ((ubo.light.flags & DL_SHOW) > 0) {
        // "s" is the direction to the light
        vec3 s = normalize(vec3(ubo.light.position) - fragPos);
        float sDotN = max(dot(s,n), 0.0);

        // Diffuse color depends on "s" and the "n" only (Lambertian)
        diff = kd * sDotN;

        // Only calculate specular if the cosine is positive
        if(sDotN > 0.0) {
            // "v" is the direction to the camera
            vec3 v = normalize(ubo.camera.position - fragPos);
            // "h" is the halfway vector between "v" & "s" (Blinn)
            vec3 h = normalize(v + s);
            spec = ks *
                    pow(max(dot(h,n), 0.0), shininess);
        }
    }

    return ambient + ubo.light.L * (diff + spec);
    // return diff;
    // return ambient;
    // return ubo.light.L * spec;
}