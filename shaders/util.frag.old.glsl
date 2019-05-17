
#version 450
#extension GL_ARB_separate_shader_objects : enable

// LIGHT FLAGS
const uint DL_HIDE          = 0x00000001u;
const uint DL_SHOW          = 0x00000002u;
const uint DL_LAMBERTIAN    = 0x00000010u;
const uint DL_BLINN_PHONG   = 0x00000020u;

// APPLICATION CONSTANTS
// TODO: get these from a ubo or something...
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space
const float Ia = 0.35; // ambient light intesity

struct Camera {
	mat4 mvp;
	vec3 position;
};

struct PositionalLight {
	mat4 model;
	vec3 color; uint flags;
	float intensity;
	float phongExp;
};

// IN
layout(location = 0) in vec3 fragPos;
// UNIFORM BUFFER
layout(binding = 0) uniform DefaultUniformBuffer {
	Camera camera;
	PositionalLight light;
} ubo;

vec4 getColor(vec3 n, vec3 ka, vec3 kd, vec3 ks, float opacity) {
    vec3 La = (ka * Ia), Ld, Ls;

    // check if light should be shown
    if ((ubo.light.flags & DL_SHOW) > 0) {

        vec3 l = normalize(vec3(ubo.light.model[3]) - fragPos);
        float I = ubo.light.intensity;

        // Lambertian
        if ((ubo.light.flags & DL_LAMBERTIAN | ubo.light.flags & DL_BLINN_PHONG) > 0) {
            /*  Lambertian shading model:

                    L = kd I max(0, n dot l)

                where "L" is the pixel color; "kd" is the diffuse coefficient,
                or the surface color; and "I" is the intensity of the light source.
                "n" is the surface normal, and "l" is a unit vector pointing toward
                the light source.
            */
            Ld = kd * I * max(0, dot(n, l));
        }

        // Blinn-Phong
        if ((ubo.light.flags & DL_BLINN_PHONG) > 0) {
            /*  Blinn Phong shading model:

                    L = kd I max(0, n dot l) + ks I max(0, n dot h)^p

                where "ks" is the specular coefficient, or the
                specular color, of the surface. "v" is the view
                direction, and "h" is the half vector between "v"
                and "l". The power, or Phong exponent, controls
                the apparent shininess of the surface. The half
                vector itself is easy to compute: since v and l
                are the same length, their sum is a vector that
                bisects the angle between them, which only needs
                to be normalized to produce h.
            */
            vec3 v = normalize(ubo.camera.position - fragPos);
            vec3 h = normalize(v + l);
            float p = ubo.light.phongExp;

            Ls = ks * I * pow(max(0, dot(n, h)), p);
        }
	}

    /*  Blinn Phong shading model with ambient lighting:

            L = ka * Ia + kd I max(0,n dot l) + ks I max(0,n dot h)^n

        where ka is the surface’s ambient coefficient, or “ambient
        color,” and Ia is the ambient light intensity.
    */
    return vec4(La + Ld  + Ls, opacity);
}