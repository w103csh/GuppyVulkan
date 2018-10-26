
#version 450
#extension GL_ARB_separate_shader_objects : enable

// DIRECTION LIGHT FLAGS
const uint DL_HIDE          = 0x00000001u;
const uint DL_SHOW          = 0x00000002u;
const uint DL_BLINN_PHONG   = 0x00000010u;
const uint DL_LAMBERTIAN    = 0x00000020u;

// DYNAMIC UNIFORM BUFFER FLAGS
// TEXTURE
const uint TEX_DIFFUSE      = 0x00000001u;
const uint TEX_NORMAL       = 0x00000010u;
const uint TEX_SPECULAR     = 0x00000100u;


// APPLICATION CONSTANTS
// TODO: get these from a ubo or something...
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

struct DirectionalLight {
    vec3 pos;   float power;
    vec3 color; float shininess;
    vec3 diff;  uint flags;
    vec3 spec;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    DirectionalLight light;
} ubo;

layout(binding = 1) uniform sampler2DArray texSampler;

layout(binding = 2) uniform DynamicUniformBufferObject {
    uint texFlags;
} dynamicUbo;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    int samplerOffset = 0;

    // Diffuse color
    vec4 diffuseColor = vec4(0, 0, 0, 1.0);
    if ((dynamicUbo.texFlags & TEX_DIFFUSE) > 0) {
        diffuseColor = texture(texSampler, vec3(fragTexCoord, samplerOffset++));
    } else {
        // TODO: what should this be? ...
    }
    outColor = diffuseColor;

    // Specular color
    vec4 specularColor = vec4(0, 0, 0, 1.0);
    if ((dynamicUbo.texFlags & TEX_SPECULAR) > 0) {
        specularColor = texture(texSampler, vec3(fragTexCoord, samplerOffset++));
    } else {
        // TODO: what should this be? ...
        specularColor = vec4(ubo.light.spec, 1.0);
    }
    
    // Normal
    vec3 n = fragNormal;
    if ((dynamicUbo.texFlags & TEX_NORMAL) > 0) {
        n = vec3(texture(texSampler, vec3(fragTexCoord, samplerOffset++)));
    }

    // check if light should be shown
    if ((ubo.light.flags & DL_SHOW) > 0) {
    
        // Lambertian
        if ((ubo.light.flags & DL_LAMBERTIAN) > 0) {

            /*  Lambertian shading model:

                    L = kd I max(0, n · l)

                where "L" is the pixel color; "kd" is the diffuse coefficient,
                or the surface color; and "I" is the intensity of the light source.
                "n" is the surface normal, and "l" is a unit vector pointing toward
                the light source.
            */

            vec3 kd = vec3(diffuseColor);
//            vec3 kd = vec3(0.4, 0.4, 0.4);
            float I = 1.4;
            vec3 l = normalize(ubo.light.pos - fragPos);
            kd *= I * max(0, dot(n, l));

            /*  Blinn Phong shading model:

                    L = kd I max(0, n · l) + ks I max(0, n · h)^p


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

//            vec3 ks = vec3(specularColor);
            vec3 ks = vec3(0.7, 0.7, 0.7);
            vec3 v = normalize(ubo.light.diff - fragPos);
            vec3 h = normalize(v + l);
            float p = 16.0;
            ks *= I * pow(max(0, dot(n, h)), p);

            outColor = vec4(kd, diffuseColor[3]) + vec4(ks, specularColor[3]);

            float lambertian = max(0, dot(n, l));

            if(lambertian > 0.0) {

//                float dist = length(lightDir);
//                dist *= dist;
//                lightDir = normalize(lightDir);
//
//                vec3 viewDir = normalize(-fragPos);
//
//                // this is blinn phong
//                vec3 halfDir = normalize(lightDir + viewDir);
//                float specAngle = max(dot(halfDir, normal), 0.0);
//                specular = pow(specAngle, ubo.dirLight1.shininess);
//       
//                // this is phong (for comparison)
//                if((ubo.dirLight1.flags & DL_BLINN_PHONG) > 0) {
//                    vec3 reflectDir = reflect(-lightDir, fragNormal);
//                    specAngle = max(dot(reflectDir, viewDir), 0.0);
//                    // note that the exponent is different here
//                    specular = pow(specAngle, ubo.dirLight1.shininess/4.0);
//                }
            }
        }

//        vec3 colorLinear = vec3(diffuseColor) +
//                            vec3(specularColor) * specular * ubo.dirLight1.color * ubo.dirLight1.power / dist;
//        // apply gamma correction (assume ambientColor, diffuseColor and specColor
//        // have been linearized, i.e. have no gamma correction in them)
//        vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0/screenGamma));
//
//        // use the gamma corrected color in the fragment
//        outColor = vec4(colorGammaCorrected, 1.0);
    }
}