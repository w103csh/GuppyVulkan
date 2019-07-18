#ifndef BUFFER_MANAGER_DESCRIPTOR_H
#define BUFFER_MANAGER_DESCRIPTOR_H

#include <memory>
#include <string>
#include <variant>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "BufferManager.h"

namespace Descriptor {

template <class TBase, class TDerived, template <typename> class TSmartPointer>
class Manager : public Buffer::Manager::Base<TBase, TDerived, TSmartPointer> {
   public:
    Manager(const std::string &&name, const DESCRIPTOR &&descriptorType, const VkDeviceSize &&maxSize,
            const bool &&keepMapped, const std::string &&macroName = "N/A",
            const VkSharingMode &&sharingMode = VK_SHARING_MODE_EXCLUSIVE, const VkBufferCreateFlags &&flags = 0)
        : Buffer::Manager::Base<TBase, TDerived, TSmartPointer>(
              //
              std::forward<const std::string>(name),                          //
              std::forward<const VkDeviceSize>(maxSize),                      //
              std::forward<const bool>(keepMapped),                           //
              std::visit(Descriptor::GetVkBufferUsage{}, descriptorType),     //
              std::visit(Descriptor::GetVkMemoryProperty{}, descriptorType),  //
              std::forward<const VkSharingMode>(sharingMode),                 //
              std::forward<const VkBufferCreateFlags>(flags)),
          DESCRIPTOR_TYPE(descriptorType),
          MACRO_NAME(macroName) {}

    const DESCRIPTOR DESCRIPTOR_TYPE;
    const std::string MACRO_NAME;

    void init(const Shell::Context &ctx, const Game::Settings &settings,
              std::vector<uint32_t> queueFamilyIndices = {}) override {
        // TODO: dump the alignment padding here so you can see how bad it is...
        const auto &limits = ctx.physicalDevProps[ctx.physicalDevIndex].properties.limits;
        if (sizeof(typename TDerived::DATA) % limits.minUniformBufferOffsetAlignment != 0) {
            Buffer::Manager::Base<TBase, TDerived, TSmartPointer>::alignment_ =
                (sizeof(typename TDerived::DATA) + limits.minUniformBufferOffsetAlignment - 1) &
                ~(limits.minUniformBufferOffsetAlignment - 1);
        }
        Buffer::Manager::Base<TBase, TDerived, TSmartPointer>::init(ctx, settings,
                                                                    std::forward<std::vector<uint32_t>>(queueFamilyIndices));
    }

   private:
    void setInfo(Buffer::Info &info) override {
        if (std::visit(Descriptor::IsUniformDynamic{}, DESCRIPTOR_TYPE)) info.bufferInfo.offset = 0;
    }
};

}  // namespace Descriptor

#endif  // !BUFFER_MANAGER_DESCRIPTOR_H
