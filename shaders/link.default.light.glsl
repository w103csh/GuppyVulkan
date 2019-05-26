
// #version 450
// #extension GL_ARB_separate_shader_objects : enable

// #define UMI_LGT_DEF_POS 0
// #define UMI_LGT_DEF_SPT 0
// #define DSMI_UNI_DEF 0

// // FLAGS
// const uint LIGHT_SHOW       = 0x00000001u;

// struct LightDefaultPositional;
// struct LightDefaultSpot;

// #if UMI_LGT_DEF_POS
// layout(set=DSMI_UNI_DEF, binding=3) uniform block1 {
//     LightDefaultPositional data;
// } lgtPos[UMI_LGT_DEF_POS];
// #endif

// #if UMI_LGT_DEF_SPT
// layout(set=DSMI_UNI_DEF, binding=4) uniform block2 {
//     LightDefaultSpot data;
// } lgtSpot[UMI_LGT_DEF_SPT];
// #endif

// bool showLight(const uint in flags) { return (flags & PER_MATERIAL_COLOR) > 0; }
// LightDefaultPositional 


