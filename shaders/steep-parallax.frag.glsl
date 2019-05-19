
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define UMI_LGT_DEF_POS 0

layout(location=0) in vec2 TexCoord;
layout(location=1) in vec3 ViewDir;
#if UMI_LGT_DEF_POS
layout(location=2) in vec3 LightDir[UMI_LGT_DEF_POS];
#endif

layout(set=1, binding=0) uniform sampler2D ColorTex;
layout(set=1, binding=1) uniform sampler2D NormalMapTex;
// layout(set=1, binding=2) uniform sampler2D HeightMapTex;

layout(set=0, binding=3) uniform LightInfo {
    vec3 Position;  // Light position in cam. coords.
    uint flags;
    vec3 La;        // Amb intensity
    vec3 L;         // D,S intensity
} Light;

layout(set=0, binding=1) uniform MaterialInfo {
    vec3 color;         // Diffuse color for dielectrics, f0 for metallic
    float opacity;      // Overall opacity
    // 16
    uint flags;         // Flags (general/material)
    uint texFlags;      // Flags (texture)
    float xRepeat;      // Texture xRepeat
    float yRepeat;      // Texture yRepeat
    // 16
    vec3 Ka;            // Ambient reflectivity
    float Shininess;    // Specular shininess factor
    // 16
    vec3 Ks;            // Specular reflectivity
    // rem 4
} Material;

layout(location=0) out vec4 FragColor;

const float bumpScale = 0.03;

vec2 findOffset(vec3 v, out float height) {
    const int nSteps = int(mix(60, 10, abs(v.z)));
    float htStep = 1.0 / nSteps;
    vec2 deltaT = (v.xy * bumpScale) / (nSteps * v.z );
    float ht = 1.0;
    vec2 tc = TexCoord.xy;
    // height = texture(HeightMapTex, tc).r;
    height = texture(NormalMapTex, tc).w;
    while( height < ht ) {
        ht -= htStep;
        tc -= deltaT;
        // height = texture(HeightMapTex, tc).r;
        height = texture(NormalMapTex, tc).w;
    }
    return tc;
}

bool isOccluded(float height, vec2 tc, vec3 s) {
    // Shadow ray cast
    const int nShadowSteps = int(mix(60,10,abs(s.z)));
    float htStep = 1.0 / nShadowSteps;
    vec2 deltaT = (s.xy * bumpScale) / ( nShadowSteps * s.z );
    float ht = height + htStep * 0.1;
    while( height < ht && ht < 1.0 ) {
        ht += htStep;
        tc += deltaT;
        // height = texture(HeightMapTex, tc).r;
        height = texture(NormalMapTex, tc).w;
    }

    return ht < 1.0;
}

vec3 blinnPhong( ) {  
    vec3 v = normalize(ViewDir);
#if UMI_LGT_DEF_POS
    vec3 s = normalize( LightDir[0] );
#else
    vec3 s = vec3(0.0);
#endif

    float height = 1.0;
    vec2 tc = findOffset(v, height);

    vec3 texColor = texture(ColorTex, tc).rgb;
    vec3 n = texture(NormalMapTex, tc).xyz;
    n.xy = 2.0 * n.xy - 1.0;
    n  = normalize(n);

    float sDotN = max( dot(s,n), 0.0 );
    vec3 diffuse = vec3(0.0), 
        ambient = Light.La * texColor,
        spec = vec3(0.0);

    if( sDotN > 0.0 && ! isOccluded(height, tc, s) ) {
        diffuse = texColor * sDotN;
        vec3 h = normalize( v + s );
        spec = Material.Ks *
                pow( max( dot(h,n), 0.0 ), Material.Shininess );
    }

    return ambient + Light.L * (diffuse + spec);
}

void main() {
    vec3 c = blinnPhong();
    c = pow(c, vec3(1.0/2.2));
    FragColor = vec4( c, 1.0 );
}
