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

struct ArcCreateInfo : public Mesh::CreateInfo {
    ArcCreateInfo() : CreateInfo{{}, false, false, PIPELINE::DEFERRED_BEZIER_4, Uniform::PASS_ALL_SET, false} {};
    std::vector<Vertex::Color> controlPoints;
};

class Arc : public Line {
    friend class Handler;

   protected:
    Arc(Handler& handler, const ArcCreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
        std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Mesh

#endif  // !ARC_H