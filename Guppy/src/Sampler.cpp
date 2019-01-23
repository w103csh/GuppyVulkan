//
//#include "Sampler.h"
//
//namespace {
//
//void getDefaultSamplerBindings(std::vector<VkDescriptorSetLayoutBinding>& bindings) {
//    // Sampler
//    VkDescriptorSetLayoutBinding binding = {};
//    binding.binding = static_cast<uint32_t>(bindings.size());
//    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    binding.descriptorCount = 1;
//    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//    binding.pImmutableSamplers = nullptr;  // Optional
//    bindings.push_back(binding);
//}
//
//}  // namespace
//
//// **********************
////      Base
//// **********************
//
//void Sampler::Base::init(const VkDevice& dev, const Game::Settings& settings, VkDeviceSize& size, uint32_t count) {
//    // Create the buffer
//    resources_.size = helpers::createBuffer(dev, (count * size), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                                            resources_.buffer, resources_.memory);
//
//    // Set resources
//    resources_.count = count;
//    resources_.info.offset = 0;
//    resources_.info.range = size;
//    resources_.info.buffer = resources_.buffer;
//
//    if (settings.enable_debug_markers) {
//        std::string markerName = name_ + " uniform buffer block";
//        ext::DebugMarkerSetObjectName(dev, (uint64_t)resources_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
//                                      markerName.c_str());
//        // TODO: what should this be???
//        ext::DebugMarkerSetObjectTag(dev, (uint64_t)resources_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
//                                     sizeof(_UniformTag), &tag);
//    }
//}
//
//void Sampler::Base::destroy(const VkDevice& dev) {
//    if (resources_.buffer != VK_NULL_HANDLE) vkDestroyBuffer(dev, resources_.buffer, nullptr);
//    if (resources_.memory != VK_NULL_HANDLE) vkFreeMemory(dev, resources_.memory, nullptr);
//}
//
//// **********************
////      Default
//// **********************
//
////void Default::createDescriptorSets(const VkDevice& dev, const VkDescriptorPool& pool, const std::shared_ptr<Texture::Data>,
////                                   std::vector<VkDescriptorSet>& sets, uint32_t imageCount, uint32_t count) {
//    // auto setCount = static_cast<uint32_t>(count * imageCount);
//    // assert(setCount > 0);
//
//    // std::vector<VkDescriptorSetLayout> layouts;
//    ////Pipeline::Handler::getDescriptorLayouts(imageCount, VERTEX_TYPE::TEXTURE, layouts);
//
//    // VkDescriptorSetAllocateInfo alloc_info;
//    // alloc_info = {};
//    // alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    // alloc_info.descriptorPool = pool;
//    // alloc_info.descriptorSetCount = setCount;
//    // alloc_info.pSetLayouts = layouts.data();
//
//    // sets.resize(setCount);
//    // vk::assert_success(vkAllocateDescriptorSets(dev, &alloc_info, sets.data()));
//
//    // std::vector<VkWriteDescriptorSet> writes;
//    // VkWriteDescriptorSet write;
//    // for (size_t i = 0; i < sets.size(); i++) {
//    //    // Default uniform
//    //    write = {};
//    //    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    //    write.dstSet = sets[i];
//    //    write.dstBinding = 0;  // !!!!
//    //    write.dstArrayElement = 0;
//    //    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    //    write.descriptorCount = 1;
//    //    write.pBufferInfo = &pRes->uboInfos[0];  // !!! hardcode
//    //    writes.push_back(write);
//    //    // Sampler
//    //    write = {};
//    //    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    //    write.dstSet = sets[i];
//    //    write.dstBinding = 1;  // !!!!
//    //    write.dstArrayElement = 0;
//    //    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    //    write.descriptorCount = 1;
//    //    write.pImageInfo = &info;
//    //    writes.push_back(write);
//    //    // Dynamic uniform (texture flags)
//    //    write = {};
//    //    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    //    write.dstSet = sets[i];
//    //    write.dstBinding = 2;  // !!!!
//    //    write.dstArrayElement = 0;
//    //    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
//    //    write.descriptorCount = 1;
//    //    write.pBufferInfo = &pRes->dynUboInfos[0];  // !!! hardcode
//    //    writes.push_back(write);
//    //}
//    // vkUpdateDescriptorSets(dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
////}
