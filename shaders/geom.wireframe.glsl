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
layout(triangles) in;
layout(location=0) in vec3 inPosition[];
layout(location=1) in vec3 inNormal[];
layout(location=2) in vec4 inColor[];
// OUT
layout(triangle_strip, max_vertices=3) out;
layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec4 outColor;
layout(location=10) noperspective out vec3 outEdgeDistance;
layout(location=11) noperspective out vec4 outEdgeColor;

void main() {
    // Transform each vertex into viewport space
    vec2 p0 = vec2(uniGeomDef.viewport * vec4(inPosition[0], 1.0));
    vec2 p1 = vec2(uniGeomDef.viewport * vec4(inPosition[1], 1.0));
    vec2 p2 = vec2(uniGeomDef.viewport * vec4(inPosition[2], 1.0));

    float a = length(p1 - p2);
    float b = length(p2 - p0);
    float c = length(p1 - p0);
    float alpha = acos((b*b + c*c - a*a) / (2.0*b*c));
    float beta = acos((a*a + c*c - b*b) / (2.0*a*c));
    float ha = abs(c * sin(beta));
    float hb = abs(c * sin(alpha));
    float hc = abs(b * sin(alpha));

    outPosition = inPosition[0];
    outNormal = inNormal[0];
    outColor = inColor[0];
    outEdgeDistance = vec3(ha, 0, 0);
    outEdgeColor = vec4(1, 0, 0, 1);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    outPosition = inPosition[1];
    outNormal = inNormal[1];
    outColor = inColor[1];
    outEdgeDistance = vec3(0, hb, 0);
    outEdgeColor = vec4(0, 1, 0, 1);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    outPosition = inPosition[2];
    outNormal = inNormal[2];
    outColor = inColor[2];
    outEdgeDistance = vec3(0, 0, hc);
    outEdgeColor = vec4(0, 0, 1, 1);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
