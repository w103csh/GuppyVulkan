/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * 
 * Parallax Shader
 */

#ifndef PARALLAX_H
#define PARALLAX_H

#include "ConstantsAll.h"
#include "DescriptorSet.h"
#include "Pipeline.h"

namespace Descriptor {
namespace Set {
namespace Parallax {
extern const CreateInfo UNIFORM_CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;
}  // namespace Parallax
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace Parallax {
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo SIMPLE_CREATE_INFO;
extern const CreateInfo STEEP_CREATE_INFO;
}  // namespace Parallax
}  // namespace Shader

namespace Pipeline {
namespace Parallax {

extern const Pipeline::CreateInfo SIMPLE_CREATE_INFO;
class Simple : public Pipeline::Graphics {
   public:
    Simple(Pipeline::Handler &handler) : Graphics(handler, &SIMPLE_CREATE_INFO) {}
};

extern const Pipeline::CreateInfo STEEP_CREATE_INFO;
class Steep : public Pipeline::Graphics {
   public:
    Steep(Pipeline::Handler &handler) : Graphics(handler, &STEEP_CREATE_INFO) {}
};
}  // namespace Parallax
}  // namespace Pipeline

#endif  // !PARALLAX_H