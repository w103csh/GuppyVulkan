
#ifndef ENUM_H
#define ENUM_H

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

// clang-format off
typedef enum STATUS {
    READY =             0x00000000,
    PENDING_VERTICES =  0x00000002,
    PENDING_BUFFERS =   0x00000004,
    PENDING_MATERIAL =  0x00000008,
    PENDING_TEXTURE =   0x00000010,
    PENDING_PIPELINE =  0x00000020,
    REDRAW =            0x00000040,
    UPDATE_BUFFERS =    0x00000080,
    PENDING =           0x7FFFFFFF,
} STATUS;
// clang-format on

enum class VERTEX {
    //
    COLOR = 0,
    TEXTURE
};

enum class PUSH_CONSTANT {
    DONT_CARE = -1,
    //
    DEFAULT = 0,
    PBR,
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

#endif  // !ENUM_H