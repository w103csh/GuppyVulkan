
#version 450

#define _DS_UNI_GEOM_DEF 0

// FLAGS
const uint WIREFRAME       = 0x00000002u;

// PUSH CONSTANTS
layout(push_constant) uniform PushBlock {
    uint flags;
} pushConstantsBlock;

// BINDINGS
layout(set=_DS_UNI_GEOM_DEF, binding=0) uniform GeometryDefault {
    mat4 viewport;
    vec4 color;
    // 0: line width
    // 1: silhouette pctExtend
    // 2: silhouette edgeWidth
    // rem
    vec4 data1;
} uniGeomDef;

#if 0
layout(location=10) noperspective in vec3 inEdgeDistance;
layout(location=11) noperspective in vec4 inEdgeColor;
layout(location=12) flat in int isEdge;
#endif

void geomShade(inout vec4 color) {
    return;
    // if (isEdge == 1) {
    //     color = uniGeomDef.color;
    //     color = vec4(1);
    // } else if ((pushConstantsBlock.flags & WIREFRAME) > 0) {
    //     // Find the smallest distance
    //     float d = min(inEdgeDistance.x, inEdgeDistance.y);
    //     d = min(d, inEdgeDistance.z);

    //     float mixVal;
    //     if (d < uniGeomDef.data1[0] - 1) {
    //         mixVal = 1.0;
    //     } else if (d > uniGeomDef.data1[0] + 1) {
    //         mixVal = 0.0;
    //     } else {
    //         float x = d - (uniGeomDef.data1[0] - 1);
    //         mixVal = exp2(-2.0 * (x*x));
    //     }
    //     color = mix(color, inEdgeColor, mixVal);
    // }
}
