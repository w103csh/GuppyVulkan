/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * //////////////////////////////////////////////////////////////////////
 * // Copyright (C) 2009 - Filip Strugar.
 * // Distributed under the zlib License (see readme.txt)
 * //////////////////////////////////////////////////////////////////////
 */
 
#version 450

#define _DS_UNI_DEF 0
#define _DS_CDLOD   0

layout(set=_DS_UNI_DEF, binding=0) uniform CameraDefaultPerspective {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 worldPosition;
} camera;

layout(set=_DS_CDLOD, binding=0) uniform CdlodDefault {
    vec4 terrainScale;
    vec4 terrainOffset;
    vec4 heightmapTextureInfo;
    vec2 worldMax;  // .xy max used to clamp triangles outside of world range
    vec2 samplerWorldToTextureScale;
} camera;

layout(push_constant) uniform PushBlock {
    vec4 quadOffset;   // .z holds the z center of the AABB
    vec4 quadScale;    // .z holds the current LOD level
    vec4 morphConsts;
    vec3 gridDim;      // .x = gridDim, .y = gridDimHalf, .z = oneOverGridDimHalf
} pc;

// IN
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in mat4 inModel;
// OUT
layout(location=0) out vec3 outPosition;    // (world space)
layout(location=1) out vec3 outNormal;      // (world space)
layout(location=2) out vec4 outColor;


// struct FixedVertexOutput
// {
//    vec4   position          : POSITION;   // vertex position 
//    vec2   tex0              : TEXCOORD0;
// };

// struct TerrainVertexOutput
// {
//    vec4   position       : POSITION;
//    vec4   globalUV_detUV : TEXCOORD0;      // .xy = globalUV, .zw = detailTexUV
//    vec4   lightDir       : TEXCOORD1;      // .xyz
//    vec4   eyeDir         : TEXCOORD2;		// .xyz = eyeDir, .w = eyeDist
//    vec4   morphInfo      : TEXCOORD3;      // .x = morphK, .y = specMorphK
// #if USE_SHADOWMAP
//    vec4   shadowPosition : TEXCOORD4;      // pos in shadow proj space
//    vec4   shadowNoise    : TEXCOORD5;      // .xy - noise UVs, .zw - offsetted depth & w
//    vec4   shadowL2Position : TEXCOORD6;    // pos in shadow L2 proj space
// #endif
// };

vec4 tex2Dlod_bilinear( const sampler tex, vec4 t, const vec2 textureSize, const vec2 texelSize )
{
	// vec4 c00 = tex2Dlod( tex, t );
	// vec4 c10 = tex2Dlod( tex, t + vec4( texelSize.x, 0.0, 0.0, 0.0 ));
	// vec4 c01 = tex2Dlod( tex, t + vec4( 0.0, texelSize.y, 0.0, 0.0 ) );
	// vec4 c11 = tex2Dlod( tex, t + vec4( texelSize.x, texelSize.y, 0.0, 0.0 ) );

	// vec2 f = frac( t.xy * textureSize );
	// vec4 c0 = lerp( c00, c10, f.x );
	// vec4 c1 = lerp( c01, c11, f.x );
	// return lerp( c0, c1, f.y );
    return vec4();
}

// returns baseVertexPos where: xy are true values, z is g_quadOffset.z which is z center of the AABB
vec4 getBaseVertexPos( vec4 inPos )
{
//    vec4 ret = inPos * g_quadScale + g_quadOffset;
//    ret.xy = min( ret.xy, g_quadWorldMax );
//    return ret;
    return vec4();
}

// morphs vertex xy from from high to low detailed mesh position
vec2 morphVertex( vec4 inPos, vec2 vertex, float morphLerpK )
{
//    vec2 fracPart = (frac( inPos.xy * vec2(g_gridDim.y, g_gridDim.y) ) * vec2(g_gridDim.z, g_gridDim.z) ) * g_quadScale.xy;
//    return vertex.xy - fracPart * morphLerpK;
    return vec2();
}

// calc big texture tex coords
vec2 calcGlobalUV( vec2 vertex )
{
//    vec2 globalUV = (vertex.xy - g_terrainOffset.xy) / g_terrainScale.xy;  // this can be combined into one inPos * a + b
//    globalUV *= g_samplerWorldToTextureScale;
//    globalUV += g_heightmapTextureInfo.zw * 0.5;
//    return globalUV;
    return vec2();
}

float sampleHeightmap( uniform sampler heightmapSampler, vec2 uv, float mipLevel, bool useFilter )
{
// #if BILINEAR_VERTEX_FILTER_SUPPORTED
   
//    return tex2Dlod( heightmapSampler, vec4( uv.x, uv.y, 0, mipLevel ) ).x;

// #else

// const vec2 textureSize = g_heightmapTextureInfo.xy;
// const vec2 texelSize   = g_heightmapTextureInfo.zw; 

//    uv = uv.xy * textureSize - vec2(0.5, 0.5);
//    vec2 uvf = floor( uv.xy );
//    vec2 f = uv - uvf;
//    uv = (uvf + vec2(0.5, 0.5)) * texelSize;

//    float t00 = tex2Dlod( heightmapSampler, vec4( uv.x, uv.y, 0, mipLevel ) ).x;
//    float t10 = tex2Dlod( heightmapSampler, vec4( uv.x + texelSize.x, uv.y, 0, mipLevel ) ).x;

//    float tA = lerp( t00, t10, f.x );

//    float t01 = tex2Dlod( heightmapSampler, vec4( uv.x, uv.y + texelSize.y, 0, mipLevel ) ).x;
//    float t11 = tex2Dlod( heightmapSampler, vec4( uv.x + texelSize.x, uv.y + texelSize.y, 0, mipLevel ) ).x;

//    float tB = lerp( t01, t11, f.x );

//    return lerp( tA, tB, f.y );

// #endif
    return 0.0;
}


void ProcessCDLODVertex( vec4 inPos /*: POSITION*/, out vec4 outUnmorphedWorldPos, out vec4 outWorldPos, out vec2 outGlobalUV, out vec2 outDetailUV, out vec2 outMorphK, out float outEyeDist )
{
//    vec4 vertex     = getBaseVertexPos( inPos );

//    const float LODLevel = g_quadScale.z;

//    // could use mipmaps for performance reasons but that will need some additional shader logic for morphing between levels, code 
//    // changes and making sure that hardware supports it, so I'll leave that for some other time
//    // pseudocode would be something like:
//    //  - first approx-sampling:      mipLevel = LODLevel - log2(RenderGridResolutionMult) (+1?);
//    //  - morphed precise sampling:   mipLevel = LODLevel - log2(RenderGridResolutionMult) + morphLerpK;
//    float mipLevel = 0;  

//    ////////////////////////////////////////////////////////////////////////////   
//    // pre-sample height to be able to precisely calculate morphing value
//    //  - we could probably live without this step if quad tree granularity is high (LeafQuadTreeNodeSize
//    //    is small, like in the range of 4-16) by calculating the approximate height from 4 quad edge heights
//    //    provided in shader consts; tweaking morph values; and possibly fixing other problems. But since that's
//    //    only an optimisation, and this version is more robust, we'll leave it in for now.
//    vec2 preGlobalUV = calcGlobalUV( vertex.xy );
//    vertex.z = sampleHeightmap( g_terrainHMVertexTexture, preGlobalUV.xy, mipLevel, false ) * g_terrainScale.z + g_terrainOffset.z;
//    //vertex.z = tex2Dlod( g_terrainHMVertexTexture, vec4( preGlobalUV.x, preGlobalUV.y, 0, mipLevel ) ).x * g_terrainScale.z + g_terrainOffset.z;

//    outUnmorphedWorldPos = vertex;
//    outUnmorphedWorldPos.w = 1;
   
//    float eyeDist     = distance( vertex, g_cameraPos );
//    float morphLerpK  = 1.0f - clamp( g_morphConsts.z - eyeDist * g_morphConsts.w, 0.0, 1.0 );   
   
//    vertex.xy         = morphVertex( inPos, vertex.xy, morphLerpK );
 
//    vec2 globalUV   = calcGlobalUV( vertex.xy );

//    vec4 vertexSamplerUV = vec4( globalUV.x, globalUV.y, 0, mipLevel );

//    ////////////////////////////////////////////////////////////////////////////   
//    // sample height and calculate it
//    vertex.z = sampleHeightmap( g_terrainHMVertexTexture, vertexSamplerUV, mipLevel, morphLerpK > 0 ) * g_terrainScale.z + g_terrainOffset.z;
//    //vertex.z = tex2Dlod( g_terrainHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
//    vertex.w = 1.0;   // this could also be set simply by having g_quadOffset.w = 1 and g_quadScale.w = 0....   
   
//    float detailMorphLerpK = 0.0;
//    vec2 detailUV   = vec2( 0, 0 );

//    outWorldPos       = vertex;
//    outGlobalUV       = globalUV;
//    outDetailUV       = detailUV;
//    outMorphK         = vec2( morphLerpK, detailMorphLerpK );
//    outEyeDist        = eyeDist;
}

void main()
{
//    TerrainVertexOutput output;
   
//    vec4 unmorphedWorldPos;
//    vec4 worldPos;
//    vec2 globalUV;
//    vec2 detailUV;
//    vec2 morphK;
//    float eyeDist;
//    ProcessCDLODVertex( inPos, unmorphedWorldPos, worldPos, globalUV, detailUV, morphK, eyeDist );

//    ////////////////////////////////////////////////////////////////////////////   
//    vec3 eyeDir     = normalize( (vec3)g_cameraPos - (vec3)worldPos );
//    vec3 lightDir   = g_diffuseLightDir;
//    //

//    //output.worldPos         = vec4( worldPos, distance( worldPos, g_cameraPos.xyz ) );
//    output.position         = mul( worldPos, g_viewProjection );
//    output.globalUV_detUV   = vec4( globalUV.x, globalUV.y, detailUV.x, detailUV.y );
//    output.eyeDir           = vec4( eyeDir, eyeDist );
//    output.lightDir         = vec4( lightDir, 0.0 );
//    output.morphInfo        = vec4( morphK.xy, 0.0, 0.0 );

//    return output;    
}
