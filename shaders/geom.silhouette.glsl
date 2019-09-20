#version 450

#define _DS_UNI_GEOM_DEF 0

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

// IN
layout(triangles_adjacency) in;
layout(location=0) in vec3 inPosition[];
layout(location=1) in vec3 inNormal[];
layout(location=2) in vec4 inColor[];
// OUT
layout(triangle_strip, max_vertices=3) out;
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;
layout(location=12) flat out int isEdge;

bool isFrontFacing(vec3 a, vec3 b, vec3 c) {
    return ((a.x * b.y - b.x * a.y) + (b.x * c.y - c.x * b.y) + (c.x * a.y - a.x * c.y)) > 0;
}

void emitEdgeQuad(vec3 e0, vec3 e1) {
    vec2 ext = uniGeomDef.data1[1] * (e1.xy - e0.xy);
    vec2 v = normalize(e1.xy - e0.xy);
    vec2 n = vec2(-v.y, v.x) * uniGeomDef.data1[2];

    isEdge = 1;   // This is part of the sil. edge

    gl_Position = vec4(e0.xy - ext, e0.z, 1.0);     // 0
    EmitVertex();
    gl_Position = vec4(e0.xy - n - ext, e0.z, 1.0); // 1
    EmitVertex();
    gl_Position = vec4(e1.xy + ext, e1.z, 1.0);     // 2
    EmitVertex();
    gl_Position = vec4(e1.xy - n + ext, e1.z, 1.0); // 3
    EmitVertex();

    EndPrimitive();
}

void main() {
    vec3 p0 = inPosition[0];
    vec3 p1 = inPosition[1];
    vec3 p2 = inPosition[2];
    vec3 p3 = inPosition[3];
    vec3 p4 = inPosition[4];
    vec3 p5 = inPosition[5];

    if(isFrontFacing(p0, p2, p4)) {
        if(!isFrontFacing(p0, p1, p2)) emitEdgeQuad(p0, p2);
        if(!isFrontFacing(p2, p3, p4)) emitEdgeQuad(p2, p4);
        if(!isFrontFacing(p4, p5, p0)) emitEdgeQuad(p4, p0);
    }

    // Output the original triangle

    isEdge = 0;   // This triangle is not part of an edge.

    outPosition = inPosition[0];
    outNormal = inNormal[0];
    outColor = inColor[0];
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    outPosition = inPosition[2];
    outNormal = inNormal[2];
    outColor = inColor[2];
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    outPosition = inPosition[4];
    outNormal = inNormal[4];
    outColor = inColor[4];
    gl_Position = gl_in[4].gl_Position;
    EmitVertex();

    EndPrimitive();
}
