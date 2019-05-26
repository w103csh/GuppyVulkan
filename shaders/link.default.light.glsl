
// #version 450
// #extension GL_ARB_separate_shader_objects : enable

// #define _U_LGT_DEF_POS 0
// #define _U_LGT_DEF_SPT 0
// #define _DS_UNI_DEF 0

// // FLAGS
// const uint LIGHT_SHOW       = 0x00000001u;

// struct LightDefaultPositional;
// struct LightDefaultSpot;

// #if _U_LGT_DEF_POS
// layout(set=_DS_UNI_DEF, binding=3) uniform block1 {
//     LightDefaultPositional data;
// } lgtPos[_U_LGT_DEF_POS];
// #endif

// #if _U_LGT_DEF_SPT
// layout(set=_DS_UNI_DEF, binding=4) uniform block2 {
//     LightDefaultSpot data;
// } lgtSpot[_U_LGT_DEF_SPT];
// #endif

// bool showLight(const uint in flags) { return (flags & PER_MATERIAL_COLOR) > 0; }
// LightDefaultPositional 


