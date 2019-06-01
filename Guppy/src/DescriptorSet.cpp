
#include "DescriptorSet.h"

#include "Constants.h"
#include "Texture.h"

Descriptor::Set::Base::Base(const DESCRIPTOR_SET&& type, const std::string&& macroName,
                            const Descriptor::bindingMap&& bindingMap)
    : TYPE(type), MACRO_NAME(macroName), BINDING_MAP(bindingMap), layout(VK_NULL_HANDLE), stages(0) {}

Descriptor::Set::Resource& Descriptor::Set::Base::getResource(const uint32_t& offset) {
    for (auto& resource : resources_)
        if (resource.offset == offset) return resource;
    // Add a resource if one for the offset doesn't exist.
    resources_.push_back({offset});
    return resources_.back();
}

Descriptor::Set::Default::Uniform::Uniform()
    : Set::Base{
          DESCRIPTOR_SET::UNIFORM_DEFAULT,
          "_DS_UNI_DEF",
          {
              {{0, 0}, {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, {0}, ""}},
              {{1, 0}, {DESCRIPTOR::MATERIAL_DEFAULT, {0}, ""}},
              {{2, 0}, {DESCRIPTOR::FOG_DEFAULT, {0}, ""}},
              {{3, 0}, {DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, {OFFSET_ALL}, ""}},
              {{4, 0}, {DESCRIPTOR::LIGHT_SPOT_DEFAULT, {OFFSET_ALL}, ""}},
          },
      } {}

Descriptor::Set::Default::Sampler::Sampler()
    : Set::Base{
          DESCRIPTOR_SET::SAMPLER_DEFAULT,
          "_DS_SMP_DEF",
          {
              {{0, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, {0}, ""}},
              //{{1, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, {0}, ""}},
              //{{2, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, {0}, ""}},
              //{{3, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, {0}, ""}},
          },
      } {}

Descriptor::Set::Default::CubeSampler::CubeSampler()
    : Set::Base{
          DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT,
          "_DS_CBE_DEF",
          {
              {{0, 0}, {DESCRIPTOR::SAMPLER_PIPELINE_COMBINED, {0}, Texture::SKYBOX_CREATE_INFO.name}},
          },
      } {}

Descriptor::Set::Default::ProjectorSampler::ProjectorSampler()
    : Set::Base{
          DESCRIPTOR_SET::PROJECTOR_DEFAULT,
          "_DS_PRJ_DEF",
          {
              //{{0, 0}, {DESCRIPTOR::SAMPLER_PIPELINE_COMBINED, {0}, Texture::STATUE_CREATE_INFO.name}},
              {{0, 0}, {DESCRIPTOR::SAMPLER_PIPELINE_COMBINED, {0}, RenderPass::TEXTURE_2D_CREATE_INFO.name}},
              {{1, 0}, {DESCRIPTOR::PROJECTOR_DEFAULT, {0}, ""}},
          },
      } {}
