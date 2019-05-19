
#ifndef ENUM_H
#define ENUM_H

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

enum class STATUS {
    //
    PENDING = 0,
    READY,
    PENDING_VERTICES,
    PENDING_BUFFERS,
    PENDING_MATERIAL,
    PENDING_TEXTURE,
    REDRAW,
    UPDATE_BUFFERS,
};

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