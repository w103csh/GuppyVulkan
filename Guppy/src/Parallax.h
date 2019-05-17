/*
    Parallax Shader

    This comes from the cookbook.
*/

#ifndef PARALLAX_H
#define PARALLAX_H

#include "DescriptorSet.h"
#include "Shader.h"
#include "Pipeline.h"

namespace Descriptor {
namespace Set {
namespace Parallax {
class Uniform : public Set::Base {
   public:
    Uniform();
};
}  // namespace Parallax
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace Parallax {
class Vertex : public Base {
   public:
    Vertex(Shader::Handler &handler);
};
class SimpleFragment : public Base {
   public:
    SimpleFragment(Shader::Handler &handler);
};
class SteepFragment : public Base {
   public:
    SteepFragment(Shader::Handler &handler);
};
}  // namespace Parallax
}  // namespace Shader

namespace Pipeline {
struct CreateInfoResources;
namespace Parallax {
class Simple : public Pipeline::Base {
   public:
    Simple(Pipeline::Handler &handler);
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};
class Steep : public Pipeline::Base {
   public:
    Steep(Pipeline::Handler &handler);
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};
}  // namespace Parallax
}  // namespace Pipeline

#endif  // !PARALLAX_H