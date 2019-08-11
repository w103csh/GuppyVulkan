
#ifndef ENUM_H
#define ENUM_H

enum class GAME_KEY {
    // virtual keys
    KEY_SHUTDOWN,
    // physical keys
    KEY_UNKNOWN,
    KEY_ESC,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SPACE,
    KEY_TAB,
    KEY_F,
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_E,
    KEY_Q,
    // number keys
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    // function keys
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    // modifiers
    KEY_ALT,
    KEY_CTRL,
    KEY_SHFT,
};

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

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
};

enum class HANDLER {
    //
    DESCRIPTOR,
    MATERIAL,
    TEXTURE,
};

enum class MATERIAL {
    //
    DEFAULT,
    PBR,
};

enum class INPUT_ACTION {
    //
    UP,
    DOWN,
    DBLCLK,
    UNKNOWN
};

enum class MESH {
    //
    COLOR = 0,
    LINE,
    TEXTURE
};

// Add new to either RenderPass::ALL & Compute::ALL
enum class PASS : uint32_t {  // TODO: make this a bitmask
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
    // Used to indicate "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
};

enum class COMBINED_SAMPLER {
    MATERIAL,
    PIPELINE,
    //
    DONT_CARE
};

enum class STORAGE_BUFFER {
    //
    POST_PROCESS,
    //
    DONT_CARE,
};

enum class STORAGE_IMAGE {
    PIPELINE,
    SWAPCHAIN,
    //
    DONT_CARE,
};

enum class UNIFORM {
    // CAMERA
    CAMERA_PERSPECTIVE_DEFAULT,
    // LIGHT
    LIGHT_POSITIONAL_DEFAULT,
    LIGHT_SPOT_DEFAULT,
    LIGHT_POSITIONAL_PBR,
    // GENERAL
    FOG_DEFAULT,
    PROJECTOR_DEFAULT,
    SCREEN_SPACE_DEFAULT,
    //
    DONT_CARE,
};

enum class UNIFORM_DYNAMIC {
    // MATERIAL
    MATERIAL_DEFAULT,
    MATERIAL_PBR,
    //
    DONT_CARE,
};

enum class INPUT_ATTACHMENT {
    POSITION,
    NORMAL,
    COLOR,
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

#endif  // !ENUM_H