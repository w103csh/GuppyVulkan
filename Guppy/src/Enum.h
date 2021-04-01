/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef ENUM_H
#define ENUM_H

#include <variant>

enum class GAME_KEY {
    UNKNOWN = 0,
    ESC,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    SPACE,
    TAB,
    F,
    W,
    A,
    S,
    D,
    E,
    Q,
    O,
    P,
    K,
    L,
    I,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    COMMA,
    PERIOD,
    SLASH,
    LEFT_SHFT,
    LEFT_CTRL,
    LEFT_ALT,
    // NUMBER KEYS
    TOP_1,
    TOP_2,
    TOP_3,
    TOP_4,
    TOP_5,
    TOP_6,
    TOP_7,
    TOP_8,
    TOP_9,
    TOP_0,
    MINUS,
    EQUALS,
    BACKSPACE,
    // FUNCTION KEYS
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    // MODIFIERS
    SHFT,
    CTRL,
    ALT,
};

// clang-format off
using GameButtonBits = uint16_t;
typedef enum GAME_BUTTON : GameButtonBits {
    NONE            = 0x0000,
    // These are 1 to 1 the constants for XInput
    DPAD_UP         = 0x0001,
    DPAD_DOWN       = 0x0002,
    DPAD_LEFT       = 0x0004,
    DPAD_RIGHT      = 0x0008,
    START           = 0x0010,
    BACK            = 0x0020,
    LEFT_THUMB      = 0x0040,
    RIGHT_THUMB     = 0x0080,
    LEFT_SHOULDER   = 0x0100,
    RIGHT_SHOULDER  = 0x0200,
    A               = 0x1000,
    B               = 0x2000,
    X               = 0x4000,
    Y               = 0x8000,
} GAME_BUTTON;
// clang-format on

// clang-format off
typedef enum STATUS {
    READY =                 0x00000000,
    PENDING_VERTICES =      0x00000001,
    PENDING_BUFFERS =       0x00000002,
    PENDING_MATERIAL =      0x00000004,
    PENDING_TEXTURE =       0x00000008,
    PENDING_PIPELINE =      0x00000010,
    PENDING_SWAPCHAIN =     0x00000020,
    PENDING_MESH =          0x00000040,
    PENDING_POINTER =       0x00000080,
    REDRAW =                0x00000100,
    UPDATE_BUFFERS =        0x00001000,
    DESTROYED =             0x00010000,
    UNKNOWN =               0x00020000,
    PENDING =               0x7FFFFFFF,
} STATUS;
// clang-format on

enum class VERTEX {
    DONT_CARE = -1,
    //
    COLOR = 0,
    TEXTURE,
    // This is TEXTURE with different attribute descriptions.
    SCREEN_QUAD,
};

enum class PUSH_CONSTANT {
    DONT_CARE = -1,
    //
    DEFAULT = 0,
    PBR,
    POST_PROCESS,
    DEFERRED,
    PRTCL_EULER,
    HFF_COLUMN,
    FFT_ROW_COL_OFFSET,
    CDLOD,
};

enum class MESH {
    DONT_CARE = -1,
    //
    COLOR = 0,
    LINE,
    TEXTURE,
};

// Add new to either RenderPass::ALL & Compute::ALL
enum class RENDER_PASS : uint32_t {  // TODO: make this a bitmask
    // SWAPCHAIN
    DEFAULT = 0,
    IMGUI,
    // SAMPLER
    SAMPLER_DEFAULT,
    SAMPLER_PROJECT,
    // SCREEN_SPACE
    SCREEN_SPACE,
    SCREEN_SPACE_HDR_LOG,
    SCREEN_SPACE_BRIGHT,
    SCREEN_SPACE_BLUR_A,
    SCREEN_SPACE_BLUR_B,
    // COMPUTE
    COMPUTE_POST_PROCESS,
    // DEFERRED
    DEFERRED,
    // SHADOW
    SHADOW,
    //
    SKYBOX_NIGHT,
    // Used to indicate "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
};

enum class COMPUTE_WORK : uint32_t {
    OCEAN,
    // Not currently used. Added for parity with RENDER_PASS.
    ALL_ENUM = UINT32_MAX,
};

enum class COMBINED_SAMPLER {
    MATERIAL,
    PIPELINE,
    PIPELINE_DEPTH,
    //
    DONT_CARE,
};

enum class STORAGE_IMAGE {
    PIPELINE,
    SWAPCHAIN,
    //
    DONT_CARE,
};

enum class STORAGE_BUFFER {
    POST_PROCESS,
    //
    DONT_CARE,
};

enum class STORAGE_BUFFER_DYNAMIC {
    PRTCL_EULER,
    PRTCL_POSITION,
    PRTCL_VELOCITY,
    PRTCL_NORMAL,
    //
    NORMAL,
    //
    DONT_CARE,
    VERTEX,  // Buffer usage only
};

enum class UNIFORM {
    // CAMERA
    CAMERA_PERSPECTIVE_DEFAULT,
    CAMERA_PERSPECTIVE_CUBE_MAP,
    // LIGHT
    LIGHT_DIRECTIONAL_DEFAULT,
    LIGHT_POSITIONAL_DEFAULT,
    LIGHT_SPOT_DEFAULT,
    LIGHT_POSITIONAL_PBR,
    LIGHT_POSITIONAL_SHADOW,
    LIGHT_CUBE_SHADOW,
    // GENERAL
    OBJ3D,
    FOG_DEFAULT,
    PROJECTOR_DEFAULT,
    SCREEN_SPACE_DEFAULT,
    DEFERRED_SSAO,
    SHADOW_DATA,
    CDLOD_QUAD_TREE,
    // TESSELLATION
    TESS_DEFAULT,
    // GEOMETRY
    GEOMETRY_DEFAULT,
    // PARTICLE
    PRTCL_WAVE,
    //
    DONT_CARE,
};

enum class UNIFORM_DYNAMIC {
    // MATERIAL
    MATERIAL_DEFAULT,
    MATERIAL_PBR,
    MATERIAL_OBJ3D,
    // PARTICLE
    PRTCL_FOUNTAIN,
    PRTCL_ATTRACTOR,
    PRTCL_CLOTH,
    //
    MATRIX_4,
    TESS_PHONG,
    CDLOD_GRID,
    // WATER
    HFF,
    OCEAN_DISPATCH,
    OCEAN_DRAW,
    //
    DONT_CARE,
};

enum class UNIFORM_TEXEL_BUFFER {
    PIPELINE,
    //
    DONT_CARE,
};

enum class INPUT_ATTACHMENT {
    POSITION,
    NORMAL,
    COLOR,
    FLAGS,
    SSAO,
    //
    DONT_CARE,
};

enum class QUEUE {
    GRAPHICS,
    PRESENT,
    TRANSFER,
    COMPUTE,
};

enum class SCENE {
    DEFAULT,
    DEFERRED,
};

enum class GRAPHICS : uint32_t {
    // DEFAULT
    TRI_LIST_COLOR = 0,
    LINE,
    POINT,
    TRI_LIST_TEX,
    CUBE,
    CUBE_MAP_COLOR,
    CUBE_MAP_LINE,
    CUBE_MAP_PT,
    CUBE_MAP_TEX,
    // PBR
    PBR_COLOR,
    PBR_TEX,
    // BLINN PHONG
    BP_TEX_CULL_NONE,
    // PARALLAX
    PARALLAX_SIMPLE,
    PARALLAX_STEEP,
    // SCREEN SPACE
    SCREEN_SPACE_DEFAULT,
    SCREEN_SPACE_HDR_LOG,
    SCREEN_SPACE_BRIGHT,
    SCREEN_SPACE_BLUR_A,
    SCREEN_SPACE_BLUR_B,
    // DEFERRED
    DEFERRED_MRT_TEX,
    DEFERRED_MRT_COLOR,
    DEFERRED_MRT_WF_COLOR,
    DEFERRED_MRT_PT,
    DEFERRED_MRT_LINE,
    DEFERRED_MRT_COLOR_RFL_RFR,
    DEFERRED_MRT_SKYBOX,
    DEFERRED_SSAO,
    DEFERRED_COMBINE,
    // SHADOW
    SHADOW_COLOR,
    SHADOW_TEX,
    // TESSELLATION
    TESS_BEZIER_4_DEFERRED,
    TESS_PHONG_TRI_COLOR_WF_DEFERRED,
    TESS_PHONG_TRI_COLOR_DEFERRED,
    // GEOMETRY
    GEOMETRY_SILHOUETTE_DEFERRED,
    // PARTICLE
    PRTCL_WAVE_DEFERRED,
    PRTCL_FOUNTAIN_DEFERRED,
    PRTCL_FOUNTAIN_EULER_DEFERRED,
    PRTCL_SHDW_FOUNTAIN_EULER,
    PRTCL_ATTR_PT_DEFERRED,
    PRTCL_CLOTH_DEFERRED,
    // HEIGHT FLUID FIELD
    HFF_CLMN_DEFERRED,
    HFF_WF_DEFERRED,
    HFF_OCEAN_DEFERRED,
    // OCEAN
    OCEAN_WF_DEFERRED,
    OCEAN_SURFACE_DEFERRED,
    // CDLOD
    CDLOD_WF_DEFERRED,
    CDLOD_TEX_DEFERRED,
    // Used to indicate bad data, and "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP in PipelineConstants.cpp
};

enum class COMPUTE : uint32_t {
    // SCREEN SPACE
    SCREEN_SPACE_DEFAULT,
    // PARTICLE
    PRTCL_EULER,
    PRTCL_ATTR,
    PRTCL_CLOTH,
    PRTCL_CLOTH_NORM,
    // HEIGHT FLUID FIELD
    HFF_HGHT,
    HFF_NORM,
    // FFT
    FFT_ONE,
    // OCEAN
    OCEAN_DISP,
    OCEAN_FFT,
    // Used to indicate bad data, and "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP in PipelineConstants.cpp
};

// VARIANTS

using DESCRIPTOR = std::variant<  //
    UNIFORM,                      //
    UNIFORM_DYNAMIC,              //
    UNIFORM_TEXEL_BUFFER,         //
    COMBINED_SAMPLER,             //
    STORAGE_IMAGE,                //
    STORAGE_BUFFER,               //
    STORAGE_BUFFER_DYNAMIC,       //
    INPUT_ATTACHMENT              //
    >;

using PIPELINE = std::variant<GRAPHICS, COMPUTE>;

using PASS = std::variant<RENDER_PASS, COMPUTE_WORK>;

#endif  // !ENUM_H