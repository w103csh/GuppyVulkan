
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _U_LGT_DEF_POS 0
#define _DS_UNI_PLX 0

// FLAGS
const uint LIGHT_SHOW       = 0x00000001u;

layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;
layout (location=2) in vec2 VertexTexCoord;
layout (location=3) in vec3 VertexTangent;
layout (location=4) in vec3 VertexBinormal;

layout(set=_DS_UNI_PLX, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} Camera;

#if _U_LGT_DEF_POS
layout(set=_DS_UNI_PLX, binding=3) uniform LightInfo {
    vec3 Position;  // Light position in cam. coords.
    uint flags;
    vec3 La;        // Amb intensity
    vec3 L;         // D,S intensity
} Light[_U_LGT_DEF_POS];
#endif

layout(location=0) out vec2 TexCoord;
layout(location=1) out vec3 ViewDir;
#if _U_LGT_DEF_POS
layout(location=2) out vec3 LightDir[_U_LGT_DEF_POS];
#endif

layout(location=5) in mat4 ModelMatrix;
// uniform mat4 ModelViewMatrix;
// uniform mat3 NormalMatrix;
// uniform mat4 MVP;

void main() {
    mat4 ModelViewMatrix = Camera.view * ModelMatrix;
    mat3 NormalMatrix = mat3(ModelViewMatrix);
    mat4 MVP = Camera.viewProjection * ModelMatrix;

    // Transform normal and tangent to eye space
    vec3 norm = normalize( NormalMatrix * VertexNormal );
    vec3 tang = normalize( NormalMatrix * VertexTangent );

    // Compute the binormal
    vec3 binormal = normalize( cross( norm, tang ) );

    // Matrix for transformation to tangent space
    mat3 toObjectLocal = transpose( mat3( tang, binormal, norm ) );

    // Transform light direction and view direction to tangent space
    vec3 pos = vec3( ModelViewMatrix * vec4(VertexPosition,1.0) );
    ViewDir = toObjectLocal * normalize(-pos);
    TexCoord = VertexTexCoord;

#if _U_LGT_DEF_POS
    for (int i = 0; i < Light.length(); i++)
        if ((Light[i].flags & LIGHT_SHOW) > 0)
            LightDir[i] = normalize( toObjectLocal * (Light[i].Position - pos) );
#endif

    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
