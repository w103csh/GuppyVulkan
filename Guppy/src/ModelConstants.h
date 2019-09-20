#ifndef MODEL_CONSTANTS_H
#define MODEL_CONSTANTS_H

#include <functional>
#include <string>

#include "MeshConstants.h"

// clang-format off
namespace Model { class Base; }
// clang-format on

namespace Model {

using index = uint32_t;
constexpr auto INDEX_MAX = UINT32_MAX;

using cback = std::function<void(Model::Base *)>;

struct CreateInfo : public Mesh::CreateInfo {
    std::string modelPath = "";
    bool async = false;
    Model::cback callback = nullptr;
};

}  // namespace Model

#endif  // !MODEL_CONSTANTS_H