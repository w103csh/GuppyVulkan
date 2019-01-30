#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Shell.h"

namespace Uniform {

class Base {
   public:
    const UNIFORM TYPE;
    const std::string NAME;

    void init(const VkDevice &dev, const Game::Settings settings);

    inline const VkDescriptorBufferInfo *getInfo() { return &resource_.info; }

    // DESCRIPTOR
    VkDescriptorSetLayoutBinding getDecriptorLayoutBinding(const VkShaderStageFlags &stageFlags, const uint32_t &binding,
                                                           uint32_t descriptorCount = 1);
    VkWriteDescriptorSet getWrite(const uint32_t &dstBinding, uint32_t dstArrayElement = 0, uint32_t descriptorCount = 1);
    // virtual VkCopyDescriptorSet getCopy() = 0;

    void destroy(const VkDevice &dev);

   protected:
    Base(const UNIFORM &&type, const VkDescriptorType &&descriptorType, const VkDeviceSize &&range, const std::string &&name)
        : TYPE(type), descriptorType_(descriptorType), NAME(name) {
        resource_.info.offset = 0;
        resource_.info.range = range;
        resource_.info.buffer = VK_NULL_HANDLE;
    }

   private:
    const VkDescriptorType descriptorType_;
    DescriptorResource resource_;
};

namespace Default {

class CameraPerspective : public Base {
   public:
    CameraPerspective()
        : Base{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(Camera),
               "Default Perspective Camera Unifrom Buffer"} {}
};

class LightPositional : public Base {
   public:
    LightPositional()
        : Base{UNIFORM::LIGHT_POSITIONAL_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(Light::Default::Positional::DATA),
               "Default Positional Light Unifrom Buffer"} {}
};

class LightSpot : public Base {
   public:
    LightSpot()
        : Base{UNIFORM::LIGHT_SPOT_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(Light::Default::Spot::DATA),
               "Default Spot Light Unifrom Buffer"} {}
};

class Material : public Base {
   public:
    Material()
        : Base{UNIFORM::MATERIAL_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, sizeof(::Material::Default::DATA),
               "Default Material Dynamic Unifrom Buffer"} {}
};

}  // namespace Default

//// **********************
////      Default
//// **********************
//
// class Default : public Base {
//   public:
//    Default::Default() : Base{DESCRIPTOR::DEFAULT_UNIFORM, "Default"} {};
//
//    typedef enum FLAG {
//        DEFAULT = 0x00000000,
//        // Should this be on the "Dynamic" uniform buffer? Now its on "Default". If it stays then
//        // move it to Scene.
//        TOON_SHADE = 0x00000001,
//        // THROUGH 0x0000000F
//        FOG_LINEAR = 0x00000010,
//        FOG_EXP = 0x00000020,
//        FOG_EXP2 = 0x00000040,
//        // THROUGH 0x0000000F (used by shader)
//        BITS_MAX_ENUM = 0x7FFFFFFF
//    } FLAG;
//
//    struct DATA {
//        const Camera::Data *pCamera = nullptr;
//        struct ShaderData {
//            alignas(16) FlagBits flags = FLAG::DEFAULT;
//            // 12 rem
//            struct Fog {
//                float minDistance = 0.0f;
//                float maxDistance = 40.0f;
//                float density = 0.05f;
//                alignas(4) float __padding1;                // rem 4
//                alignas(16) glm::vec3 color = CLEAR_COLOR;  // rem 4
//            } fog;
//        } shaderData;
//        std::vector<Light::PositionalData> positionalLightData;
//        std::vector<Light::SpotData> spotLightData;
//    } data;
//
//    const uint32_t ARRAY_ELEMENT = 0;  // Force any shaders that use this uniform to use array element 0. For now...
//
//    void init(const VkDevice &dev, const Game::Settings &settings, const Camera &camera);
//    void update(const VkDevice &dev, Camera &camera);
//
//    VkDescriptorSetLayoutBinding getDecriptorLayoutBinding(
//        uint32_t binding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_UNIFORM), uint32_t count = 1) const override;
//
//    VkWriteDescriptorSet getWrite() override;
//    VkCopyDescriptorSet getCopy() override;
//
//    void uniformAction(std::function<void(DATA &)> &&updateFunc) { updateFunc(data); }
//
//    // LIGHTS
//    uint16_t &&getPositionLightCount() { return std::move(static_cast<uint16_t>(positionalLights_.size())); }
//    uint16_t &&getSpotLightCount() { return std::move(static_cast<uint16_t>(spotLights_.size())); }
//    void lightsAction(std::function<void(std::vector<Light::Positional> &, std::vector<Light::Spot> &)> &&updateFunc) {
//        updateFunc(positionalLights_, spotLights_);
//    }
//
//   private:
//    void copy(const VkDevice &dev);
//
//    // LIGHTS
//    std::vector<Light::Positional> positionalLights_;
//    std::vector<Light::Spot> spotLights_;
//};
//
//// **********************
////      DefaultDynamic
//// **********************
//
// class DefaultDynamic : public Base {
//   public:
//    DefaultDynamic() : Base{DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM, "Default Dynamic"} {};
//
//    const uint32_t ARRAY_ELEMENT = 0;  // Force any shaders that use this uniform to use array element 0. For now...
//
//    void init(const Shell::Context &ctx, const Game::Settings &settings, uint32_t count);
//    void update(const VkDevice &dev);
//
//    VkDescriptorSetLayoutBinding getDecriptorLayoutBinding(
//        uint32_t binding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM),
//        uint32_t count = 1) const override;
//
//    VkWriteDescriptorSet getWrite() override;
//    VkCopyDescriptorSet getCopy() override;
//
//   private:
//    std::array<FlagBits, TEXTURE_LIMIT> textureFlags_;
//};

}  // namespace Uniform

#endif  // !UNIFORMBUFFER_H