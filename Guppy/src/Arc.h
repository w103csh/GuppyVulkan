/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef ARC_H
#define ARC_H

#include <glm/glm.hpp>
#include <memory>

#include "Instance.h"
#include "Mesh.h"
#include "PipelineConstants.h"
#include "Vertex.h"

namespace Mesh {

class Handler;

struct ArcCreateInfo : public CreateInfo {
    ArcCreateInfo() : CreateInfo{GRAPHICS::TESS_BEZIER_4_DEFERRED, false} {};
    std::vector<Vertex::Color> controlPoints;
};

class Arc : public Line {
    friend class Handler;

   protected:
    Arc(Handler& handler, const index&& offset, const ArcCreateInfo* pCreateInfo,
        std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Mesh

#endif  // !ARC_H