
#version 450
#extension GL_ARB_separate_shader_objects : enable

// LIGHT FLAGS
const uint POSLIGHT_SHOW    = 0x00000001u;

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
	PositionalLight lights[2];
} ubo;

vec3 n, Ka, Kd, Ks;

vec3 phongModel(PositionalLight light, uint shininess) {
    vec3 ambient = light.La * Ka;
    // "s" is the direction to the light
    vec3 s = normalize(vec3(light.position) - fragPos);
    float sDotN = max(dot(s,n), 0.0);

    // Diffuse color depends on "s" and the "n" only (Lambertian)
    vec3 diff = Kd * sDotN;

    vec3 spec = vec3(0.0);
    // Only calculate specular if the cosine is positive
    if(sDotN > 0.0) {
        // "v" is the direction to the camera
        vec3 v = normalize(ubo.camera.position - fragPos);
        // "h" is the halfway vector between "v" & "s" (Blinn)
        vec3 h = normalize(v + s);

        spec = Ks *
                pow(max(dot(h,n), 0.0), shininess);
    }

    return ambient + light.L * (diff + spec);
}

vec3 getColor(uint shininess) {
    vec3 color = vec3(0.0);

    for (int i = 0; i < ubo.lights.length(); i++) {
        if ((ubo.lights[i].flags & POSLIGHT_SHOW) > 0) {
            color += phongModel(ubo.lights[i], shininess);
        }
    }

    return color;
}