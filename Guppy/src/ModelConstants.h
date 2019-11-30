/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MODEL_CONSTANTS_H
#define MODEL_CONSTANTS_H

#include <functional>
#include <string>

#include "MeshConstants.h"

namespace Model {

class Base;

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