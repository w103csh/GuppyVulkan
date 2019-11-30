/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef AXES_H
#define AXES_H

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Instance.h"
#include "Mesh.h"

namespace Mesh {

class Handler;

const float AXES_MAX_SIZE = 2000.0f;

struct AxesCreateInfo : public CreateInfo {
    AxesCreateInfo() : CreateInfo{GRAPHICS::LINE, false} {};
    float lineSize = 1.0f;
    bool showNegative = false;
};

class Axes : public Line {
    friend class Handler;

   protected:
    Axes(Handler& handler, const index&& offset, const AxesCreateInfo* pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
    Axes(Handler& handler, const index&& offset, const std::string&& name, const AxesCreateInfo* pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Mesh

#endif  // !AXES_H