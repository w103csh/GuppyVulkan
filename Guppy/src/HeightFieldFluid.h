#ifndef HEIGHT_FIELD_FLUID_H
#define HEIGHT_FIELD_FLUID_H

#include <memory>
#include <string_view>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "ParticleBuffer.h"
#include "Pipeline.h"
#include "Plane.h"
#include "Texture.h"

namespace HeightFieldFluid {
struct Info {
    uint32_t M = 2;  // array dimension 0 size
    uint32_t N = 2;  // array dimension 1 size
    // The columns should be square in x, y to simplify the calculations, so the length of a single dimension is all that is
    // necessary.
    float lengthM = 1.0f;
};
}  // namespace HeightFieldFluid

// SHADER
namespace Shader {
extern const CreateInfo HFF_COMP_CREATE_INFO;
extern const CreateInfo HFF_CLMN_VERT_CREATE_INFO;
}  // namespace Shader

// TEXTURE
namespace Texture {
constexpr std::string_view HFF_ID = "Height Field Fluid Storage Image";
}  // namespace Texture

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace HeightFieldFluid {

namespace Simulation {
struct DATA {
    float c2;        // wave speed
    float h;         // column width/height
    float h2;        // h squared
    float dt;        // time delta
    float maxSlope;  // clamped sloped to prevent numerical explosion
    int read, write;
    int mMinus1, nMinus1;
};
struct CreateInfo : Buffer::CreateInfo {
    ::HeightFieldFluid::Info info;
    float c = 1.0f;
    float maxSlope = 1.0f;
};
class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
    void setWaveSpeed(const float c, const uint32_t frameIndex);
    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;

   private:
    float c_;
};
}  // namespace Simulation

}  // namespace HeightFieldFluid
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo HFF_CREATE_INFO;
extern const CreateInfo HFF_CLMN_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {

class Handler;

class HeightFieldFluidCompute : public Compute {
   public:
    HeightFieldFluidCompute(Handler& handler);
};

class HeightFieldFluidColumn : public Graphics {
   public:
    struct InstanceData {
        glm::vec3 position;
        glm::ivec2 imageOffset;
    };

    const bool DO_BLEND;
    const bool IS_DEFERRED;

    HeightFieldFluidColumn(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Pipeline

// BUFFER
namespace Particle {
class Handler;
namespace Buffer {
namespace HeightFieldFluid {

struct CreateInfo : Buffer::CreateInfo {
    ::HeightFieldFluid::Info info;
};

class Base : public Buffer::Base {
   public:
    Base(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
         const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors);

    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

    void init();

   private:
    std::vector<Pipeline::HeightFieldFluidColumn::InstanceData> columnData_;
    BufferResource columnRes_;

    std::unique_ptr<LoadingResource> pLdgRes2_;
};

}  // namespace HeightFieldFluid
}  // namespace Buffer
}  // namespace Particle

#endif  // !HEIGHT_FIELD_FLUID_H
