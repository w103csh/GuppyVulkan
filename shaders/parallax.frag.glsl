
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define _U_LGT_DEF_POS 0
#define _DS_UNI_PLX 0
#define _DS_SMP_PLX 0

// DECLARATIONS
vec3 getMaterialSpecular();
float getMaterialShininess();

layout(location=0) in vec2 TexCoord;
layout(location=1) in vec3 ViewDir;
#if _U_LGT_DEF_POS
layout(location=2) in vec3 LightDir[_U_LGT_DEF_POS];
#endif

#if _U_LGT_DEF_POS
layout(set=_DS_UNI_PLX, binding=3) uniform LightInfo {
    vec3 Position;  // Light position in cam. coords.
    uint flags;
    vec3 La;        // Amb intensity
    vec3 L;         // D,S intensity
} Light[_U_LGT_DEF_POS];
#endif

layout(set=_DS_SMP_PLX, binding=0) uniform sampler2D ColorTex;
layout(set=_DS_SMP_PLX, binding=1) uniform sampler2D NormalMapTex;
// layout(set=1, binding=2) uniform sampler2D HeightMapTex;

layout(location=0) out vec4 FragColor;

const uint TEST_1 = 0x01000000u;
const uint TEST_2 = 0x02000000u;
const uint TEST_3 = 0x04000000u;
const uint TEST_4 = 0x08000000u;

const int TexCoordShadeFactor = 5;
vec3 texCoordShade() {
    float modX = floor(mod((TexCoord.x * TexCoordShadeFactor), 2.0));
    float modY = floor(mod((TexCoord.y * TexCoordShadeFactor), 2.0));
    float total = modX + modY;
    if (total == 1.0) {
        return vec3(1.0);
    } else {
        return vec3(0.45);
    }
}

bool CheckerBoardTest = true;
void setCheckerBoardTest() {
    float modX = floor(mod((TexCoord.x * TexCoordShadeFactor), 2.0));
    float modY = floor(mod((TexCoord.y * TexCoordShadeFactor), 2.0));
    float total = modX + modY;
    CheckerBoardTest = (total == 1.0);
}

vec3 blinnPhong( ) {
    vec3 v = normalize(ViewDir);

    vec3 c = vec3(0.0);
    vec2 tc = TexCoord;
    if (CheckerBoardTest) {
        const float bumpFactor = 0.015;
        // float height = 1 - texture(HeightMapTex, TexCoord).r;
        float height = 1 - texture(NormalMapTex, TexCoord).w;
        vec2 delta = vec2(v.x, v.y) * height * bumpFactor / v.z;
        tc = TexCoord.xy - delta;
        //tc = TexCoord.xy;
    } else if ((Light[0].flags & TEST_3) > 0) {
        float height = 1 - texture(NormalMapTex, tc).w;
        c = vec3(height);
        return c;
    } 

    // const float bumpFactor = 0.015;
    // float height = 1 - texture(HeightMapTex, TexCoord).r;
    // vec2 delta = vec2(v.x, v.y) * height * bumpFactor / v.z;
    // vec2 tc = TexCoord.xy - delta;
    // //tc = TexCoord.xy;

    vec3 n = texture(NormalMapTex, tc).xyz;
    n.xy = 2.0 * n.xy - 1.0;
    n  = normalize(n);

    // vec3 c = vec3(0.0);

#if _U_LGT_DEF_POS
    for (int i = 0; i < LightDir.length(); i++) {
        if (i > 0) continue;

        vec3 s = normalize( LightDir[i] );

        float sDotN = max( dot(s,n), 0.0 );

        vec3 texColor = texture(ColorTex, tc).rgb;
        vec3 ambient = Light[i].La * texColor;
        vec3 diffuse = texColor * sDotN;
        vec3 spec = vec3(0.0);
        if( sDotN > 0.0 ) {  
            vec3 h = normalize( v + s );
            spec = getMaterialSpecular() *
                pow( max( dot(h,n), 0.0 ), getMaterialShininess() );
        }
        c += ambient + Light[i].L * (diffuse + spec);

    }
#endif

    return c;
}

void main() {
    if ((Light[0].flags & TEST_1) > 0) {
        FragColor = vec4(texCoordShade(), 1.0);
        return;
    } else if ((Light[0].flags & (TEST_2|TEST_3)) > 0) {
        setCheckerBoardTest();
    }
    vec3 c = blinnPhong();
    // c = pow(c, vec3(1.0/2.2));
    FragColor = vec4( c, 1.0 );
}
