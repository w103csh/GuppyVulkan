#ifndef AXES_H
#define AXES_H

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Mesh.h"

// clang-format off
namespace Mesh { class Handler; }
// clang-format on

const float AXES_MAX_SIZE = 2000.0f;

struct AxesCreateInfo : public Mesh::CreateInfo {
    AxesCreateInfo() : Mesh::CreateInfo{false, glm::mat4{1.0f}, PIPELINE::LINE, false} {};
    float lineSize = 1.0f;
    bool showNegative = false;
};

class Axes : public Mesh::Line {
    friend class Mesh::Handler;

   protected:
    Axes(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial);
    Axes(Mesh::Handler& handler, const std::string&& name, AxesCreateInfo* pCreateInfo,
         std::shared_ptr<Material::Base>& pMaterial);
};

#endif  // !AXES_H