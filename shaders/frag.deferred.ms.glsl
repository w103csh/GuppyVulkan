/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */
 
 #version 450

#define _DS_UNI_DFR_COMB 0
#define _DS_SHDW_CUBE_ALL 0
#define _DS_SMP_DFR 0

#define _U_LGT_DEF_DIR 0
#define _U_LGT_DEF_SPT 0
#define _U_LGT_SHDW_CUBE 0

// LIGHT FLAGS
const uint LIGHT_SHOW       = 0x00000001u;
// INPUT ATTACHMENT FLAGS
const uint IA_FLAT_SHADE    = 0x01u;
const uint IA_CAMERA_SPACE  = 0x02u;

layout(constant_id=0) const int NUM_SAMPLES = 8;

layout(set=_DS_UNI_DFR_COMB, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

layout(set=_DS_SHDW_CUBE_ALL, binding=0) uniform samplerCubeArrayShadow sampShadowArray;
layout(set=_DS_SHDW_CUBE_ALL, binding=1) uniform sampler3D sampShadowOffset;

layout(input_attachment_index=0, set=_DS_SMP_DFR, binding=0) uniform subpassInputMS posInput;
layout(input_attachment_index=1, set=_DS_SMP_DFR, binding=1) uniform subpassInputMS normInput;
layout(input_attachment_index=2, set=_DS_SMP_DFR, binding=2) uniform subpassInputMS diffInput;
layout(input_attachment_index=3, set=_DS_SMP_DFR, binding=3) uniform subpassInputMS ambInput;
layout(input_attachment_index=4, set=_DS_SMP_DFR, binding=4) uniform subpassInputMS specInput;
layout(input_attachment_index=5, set=_DS_SMP_DFR, binding=5) uniform usubpassInputMS flagInput;
// layout(input_attachment_index=6, set=_DS_SMP_DFR, binding=6) uniform subpassInputMS ssaoDataInput;

// IN
layout(location=0) in vec2 inTexCoord;
// OUT
layout(location=0) out vec4 outColor;

vec4 resolve(const in subpassInputMS attachment) {
    vec4 result = vec4(0.0);	   
	for (int i = 0; i < NUM_SAMPLES; i++) {
		vec4 val = subpassLoad(attachment, i);
		result += val;
	}    
	// Average resolved samples
	return result / float(NUM_SAMPLES);
}

#if _U_LGT_DEF_DIR
layout(set=_DS_UNI_DFR_COMB, binding=2) uniform LightDefaultDirectional {
    vec3 direction; // Direction to the light (s) (camera space)
    uint flags;
    vec3 La;        // Ambient light intesity
    vec3 L;         // Diffuse and specular light intensity
} lgtDir[_U_LGT_DEF_DIR];
vec3 globalDirectionalLight(
    const in vec3 norm,
    const in vec3 v,
    const in vec3 diff,
    const in vec3 amb,
    const in vec3 spec,
    const in float shininess
) {
    // Just do a basic blinn-phong shade for now.
    vec3 Ka = lgtDir[0].La * amb;
    vec3 Kd = vec3(0.0), Ks = vec3(0.0);
    float sDotN = max(dot(lgtDir[0].direction, norm), 0.0);
    Kd = diff * sDotN;
    if(sDotN > 0.0) {
        // "h" is the halfway vector between "v" & "s" (Blinn)
        vec3 h = normalize(v + lgtDir[0].direction);
        Ks = spec * pow(max(dot(h, norm), 0.0), shininess);
    }
    return Ka + lgtDir[0].L * (Kd + Ks);
}
#else
vec3 globalDirectionalLight(
    const in vec3 norm,
    const in vec3 v,
    const in vec3 diff,
    const in vec3 amb,
    const in vec3 spec,
    const in float shininess
) {
    return vec3(0);
}
#endif

#if _U_LGT_DEF_POS
layout(set=_DS_UNI_DFR_COMB, binding=3) uniform LightDefaultPositional {
    vec3 position;  // Light position in eye coords.
    uint flags;
    // 16
    vec3 La;        // Ambient light intesity
    // rem 4
    vec3 L;         // Diffuse and specular light intensity
    // rem 4
} lgtPos[_U_LGT_DEF_POS];
vec3 blinnPhongPositional(
    vec3 pos,
    vec3 norm,
    vec3 v,
    vec3 diff,
    vec3 amb,
    vec3 spec,
    float shininess
    ) {

    vec3 color = vec3(0.0);

    for (int i = 0; i < lgtPos.length(); i++) {
        if ((lgtPos[i].flags & LIGHT_SHOW) > 0) {

            vec3 Ka = pow((lgtPos[i].La * amb), vec3(i+1));
            vec3 s = normalize(lgtPos[i].position - pos);

            float sDotN = max(dot(s, norm), 0.0);
            vec3 Kd = diff * sDotN;

            vec3 Ks = vec3(0.0);
            if(sDotN > 0.0) {
                vec3 h = normalize(v + s);
                Ks = spec * pow(max(dot(h, norm), 0.0), shininess);
            }

            color += Ka + lgtPos[i].L * (Kd + Ks);
        }
    }

    return color;

}
#endif

#if _U_LGT_SHDW_CUBE
const int CUBE_LAYERS = 6;
layout(set=_DS_SHDW_CUBE_ALL, binding=2) uniform LightShadowCube {
    mat4 viewProj[CUBE_LAYERS]; // cube map mvps
    vec4 data0;                 // xyz: world position, w: cutoff
    vec3 cameraPosition;        //
    uint flags;                 //
    vec3 La;                    // Ambient intensity
    vec3 L;                     // Diffuse and specular intensity
} lgtShdwCube[_U_LGT_SHDW_CUBE];

int determineCubeFace(const vec3 v) {
    vec3 vAbs = abs(v);
    if (vAbs.z >= vAbs.x && vAbs.z >= vAbs.y) {
        return v.z < 0.0 ? 5 : 4;
    } else if (vAbs.y >= vAbs.x) {
        return v.y < 0.0 ? 3 : 2;
    } else {
        return v.x < 0.0 ? 1 : 0;
    }
}

vec3 blinnPhongCubeShadow(
    const vec3 posWS,
    const vec3 posCS,
    const vec3 normCS,
    const vec3 v,
    const vec3 diff,
    const vec3 spec,
    const float shininess
    ) {

    vec3 color = vec3(0.0);

    for (int i = 0; i < lgtShdwCube.length(); i++) {
        if ((lgtShdwCube[i].flags & LIGHT_SHOW) > 0) {

            float dist = distance(posCS, lgtShdwCube[i].cameraPosition);
            if (dist > lgtShdwCube[i].data0.w)
                continue;

            vec3 s = normalize(lgtShdwCube[i].cameraPosition - posCS);

            float sDotN = max(dot(s, normCS), 0.0);
            vec3 Kd = diff * sDotN;

            vec3 Ks = vec3(0.0);
            if(sDotN > 0.0) {
                vec3 h = normalize(v + s);
                Ks = spec * pow(max(dot(h, normCS), 0.0), shininess);
            }

            vec4 texCoord = vec4(posWS - lgtShdwCube[i].data0.xyz, i);
            int face = determineCubeFace(texCoord.xyz);
            // if (face == 0) {
            //     // color = vec3(1, 0, 0);
            // } else if (face == 1) {
            //     // color = vec3(1, 1, 0);
            //     // texCoord.z = -texCoord.z;
            // } else if (face == 2) {
            //     // color = vec3(1, 1, 1);
            //     // texCoord.y = -texCoord.y;
            // } else if (face == 3) {
            //     // color = vec3(1, 0, 1);
            //     // texCoord.x = -texCoord.x;
            // } else if (face == 4) {
            //     // color = vec3(0, 1, 0);
            //     // texCoord.x = -texCoord.x;
            // } else if (face == 5) {
            //     // color = vec3(0, 0, 1);
            // } else {
            //     color = vec3(0.0);
            // }
            // // continue;

            /**
             * This was super hard to figure out. The following formulas come directly form
             * the Vulkan specification. They are described kind of in order starting here:
             * https://www.khronos.org/registry/vulkan/specs/1.0/html/chap23.html#vertexpostproc-coord-transform
             *
             * Clip coordinates: (gl_Postion (GLSL) / Position (SPIR-V))
             *  xc
             *  yc
             *  zc
             *  wc
             *
             * Normalized device coordinates:
             *  xd = xc / wc
             *  yd = yc / wc
             *  zd = zc / wc
             *
             * Framebuffer coords: (see below)
             *  xf = (px / 2) xd + ox
             *  yf = (py / 2) yd + oy
             *  zf = pz × zd + oz
             *
             * Viewport parameters: (used above, and come from VkViewport struct defined for VkRenderPass)
             *  ox = x + width / 2
             *  oy = y + height / 2
             *  oz = minDepth
             *  px = width
             *  py = height
             *  pz = maxDepth - minDepth
             */

            vec4 clipPos = lgtShdwCube[i].viewProj[face] * vec4(posWS, 1.0);
            // Skipping the other calculations to get the right compare value here. It really should come from
            // zf = pz × zd + oz described in the comment above. The 0.0001f is to remove shadow acne caused by
            // false positives from the precision in the compare calculation.
            float compare = clipPos.z / clipPos.w - 0.001f;

            float shadow = texture(sampShadowArray, texCoord, compare);

            // Skipping filtering because cubemaps appear to not be filter friendly.

            color += lgtShdwCube[i].L * (Kd + Ks) * mix(shadow, 0, (dist / lgtShdwCube[i].data0.w));
        }
    }

    return color;

}
#endif

void main() {
    const uint flagInput_ = subpassLoad(flagInput, gl_SampleID).r;
    const vec4 diffInput_ = subpassLoad(diffInput, gl_SampleID);

    // Using the opacity at this point causes a blend with the background color...
    float alpha = true ? 1.0 : diffInput_.a;

    // Flat shade
    if ((flagInput_ & IA_FLAT_SHADE) > 0) {
        outColor = vec4(diffInput_.rgb, alpha);
        return;
    }

    const vec4 posInput_ = subpassLoad(posInput, gl_SampleID);      // world space position
    const vec4 normInput_ = subpassLoad(normInput, gl_SampleID);    // world space position
    const vec4 ambInput_ = subpassLoad(ambInput, gl_SampleID);
    const vec4 specInput_ = subpassLoad(specInput, gl_SampleID);
    // float ssaoData = subpassLoad(ssaoDataInput)[0];

    vec3 posWS,     // world space position
        posCS,      // camera space position
        normWS,     // world space normal
        normCS;     // camera space normal      

    if ((flagInput_ & IA_CAMERA_SPACE) > 0) {
        posCS = posInput_.xyz;
        normCS = normInput_.xyz;
        mat4 viewModel = inverse(camera.view);
        posWS = (viewModel * vec4(posCS, 1.0)).xyz;
        normWS = mat3(viewModel) * normCS;
    } else {
        posWS = posInput_.xyz;
        normWS = normInput_.xyz;
        posCS = (camera.view * vec4(posWS, 1.0)).xyz;
        normCS = mat3(camera.view) * normWS;
    }

    vec3 v = normalize(-posCS.xyz); // camera space direction to camera

    vec3 color;
    if (true) {
        color = globalDirectionalLight(
            normCS.xyz,
            v,
            diffInput_.rgb,
            // Ambient colors are all grey atm, so using the ambient makes its look bad.
            diffInput_.rgb, // ambInput_.rgb, 
            specInput_.rgb,
            normInput_.w
        );
        // outColor = vec4(color, alpha);
        // return;
    } else {
        color = vec3(0);
    }

#if _U_LGT_SHDW_CUBE

    float La = 0.2;
    vec3 Ka = La * mix(ambInput_.rgb, diffInput_.rgb, 0.5); // ambInput_ has no color
    vec3 KdKs = blinnPhongCubeShadow(
        posWS,
        posCS,
        normCS,
        v,
        diffInput_.rgb,
        specInput_.rgb,
        normInput_.w
    );

    color += (Ka + KdKs);

#elif _U_LGT_DEF_SPT

    color += blinnPhongSpot(
        posCS,
        normCS,
        v,
        diffInput_.rgb,
        ambInput_.rgb,
        specInput_.rgb,
        normInput_.w
    );

#endif

    outColor = vec4(color, alpha);
}