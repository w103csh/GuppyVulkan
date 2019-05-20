
// #version 450

// // **********************
// //      Flags
// // **********************

// // APPLICATION CONSTANTS
// // TODO: get these from a ubo or something...
// const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

// // LIGHT
// const uint LIGHT_SHOW       = 0x00000001u;
// // MATERIAL
// const uint PER_MATERIAL_COLOR   = 0x00000001u;
// const uint PER_VERTEX_COLOR     = 0x00000002u;
// const uint PER_TEXTURE_COLOR    = 0x00000004u;
// const uint MODE_TOON_SHADE  = 0x00000040u;
// // TEXTURE
// const uint TEX_DIFFUSE      = 0x00000001u;
// const uint TEX_NORMAL       = 0x00000010u;
// const uint TEX_SPECULAR     = 0x00000100u;
// // FOG
// const uint FOG_LINEAR       = 0x00000001u;
// const uint FOG_EXP          = 0x00000002u;
// const uint FOG_EXP2         = 0x00000004u;
// const uint _FOG_SHOW        = 0x0000000Fu;

// // **********************
// //      Structures
// // **********************

// // CAMERA
// struct CameraDefaultPerspective {
//     mat4 view;
//     mat4 projection;
//     mat4 viewProjection;
// };

// // MATERIAL
// struct MaterialDefault {
//     vec3 Ka;            // Ambient reflectivity
//     uint flags;         // Flags (general/material)
//     vec3 Kd;            // Diffuse reflectivity
//     float opacity;      // Overall opacity
//     vec3 Ks;            // Specular reflectivity
//     float shininess;    // Specular shininess factor
//     uint texFlags;      // Texture flags
//     float xRepeat;      // Texture xRepeat
//     float yRepeat;      // Texture yRepeat
//     float _padding;
// };

// // LIGHT
// struct LightDefaultPositional {
//     vec3 position;  // Light position in eye coords.
//     uint flags;
//     vec3 La;        // Ambient light intesity
//     vec3 L;         // Diffuse and specular light intensity
// };
// struct LightDefaultSpot {
//     vec3 position;
//     uint flags;
//     vec3 La;
//     float exponent;
//     vec3 L;
//     float cutoff;
//     vec3 direction;
//     uint _padding;
// };

// // FOG
// struct UniformDefaultFog {
//     float minDistance;
//     float maxDistance; 
//     float density;
//     uint flags;
//     vec3 color; // rem 4
// };


// layout (binding = 0) UniformDefaultFog uni;