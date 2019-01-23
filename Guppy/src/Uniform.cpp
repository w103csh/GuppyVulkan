
#include "Uniform.h"

#include "PipelineHandler.h"
#include "TextureHandler.h"

namespace {

void createLights(std::vector<Light::Positional>& positionalLights_, std::vector<Light::Spot>& spotLights_) {
    // TODO: move the light code into the scene, including ubo data
    positionalLights_.push_back(Light::Positional());
    positionalLights_.back().transform(helpers::affine(glm::vec3(1.0f), glm::vec3(20.5f, 10.5f, -23.5f)));

    positionalLights_.push_back(Light::Positional());
    positionalLights_.back().transform(helpers::affine(glm::vec3(1.0f), {-2.5f, 4.5f, -1.5f}));

    // positionalLights_.push_back(Light::Positional());
    // positionalLights_.back().transform(helpers::affine(glm::vec3(1.0f), glm::vec3(-10.0f, 30.0f, 6.0f)));

    spotLights_.push_back({});
    spotLights_.back().transform(helpers::viewToWorld({0.0f, 4.5f, 1.0f}, {0.0f, 0.0f, -1.5f}, UP_VECTOR));
    spotLights_.back().setCutoff(glm::radians(25.0f));
    spotLights_.back().setExponent(25.0f);

    assert(positionalLights_.size() < 255 && spotLights_.size() < 255);
}

}  // namespace

// **********************
//      Base
// **********************

void Uniform::Base::init(const VkDevice& dev, const Game::Settings& settings, VkDeviceSize& size, uint32_t count) {
    // Create the buffer
    resources_.size = helpers::createBuffer(dev, (count * size), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            resources_.buffer, resources_.memory);

    // Set resources
    resources_.count = count;
    resources_.info.offset = 0;
    resources_.info.range = size;
    resources_.info.buffer = resources_.buffer;

    if (settings.enable_debug_markers) {
        std::string markerName = name + " uniform buffer block";
        ext::DebugMarkerSetObjectName(dev, (uint64_t)resources_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      markerName.c_str());
        // TODO: what should this be???
        ext::DebugMarkerSetObjectTag(dev, (uint64_t)resources_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
                                     sizeof(_UniformTag), &tag);
    }
}

void Uniform::Base::destroy(const VkDevice& dev) {
    if (resources_.buffer != VK_NULL_HANDLE) vkDestroyBuffer(dev, resources_.buffer, nullptr);
    if (resources_.memory != VK_NULL_HANDLE) vkFreeMemory(dev, resources_.memory, nullptr);
}

// **********************
//      Default
// **********************

void Uniform::Default::init(const VkDevice& dev, const Game::Settings& settings, const Camera& camera) {
    // lights
    createLights(positionalLights_, spotLights_);
    data.positionalLightData.resize(positionalLights_.size());
    data.spotLightData.resize(spotLights_.size());

    VkDeviceSize size = 0;
    // camera
    size += sizeof(Camera::Data);
    // shader
    size += sizeof(Uniform::Default::DATA::ShaderData);
    // lights
    size += sizeof(Light::PositionalData) * data.positionalLightData.size();
    size += sizeof(Light::SpotData) * data.spotLightData.size();

    Base::init(dev, settings, size);
}

void Uniform::Default::update(const VkDevice& dev, Camera& camera) {
    // auto x1 = sizeof(DefaultUBO);
    // auto x2 = sizeof(DefaultUBO::camera);
    // auto x3 = sizeof(DefaultUBO::light);
    // auto x4 = offsetof(DefaultUBO, camera);
    // auto x5 = offsetof(DefaultUBO, light);

    // auto y1 = sizeof(Light::Ambient::Data);
    // auto y2 = offsetof(Light::Ambient::Data, model);
    // auto y3 = offsetof(Light::Ambient::Data, color);
    // auto y4 = offsetof(Light::Ambient::Data, flags);
    // auto y5 = offsetof(Light::Ambient::Data, intensity);
    // auto y6 = offsetof(Light::Ambient::Data, phongExp);

    data.pCamera = camera.getData();

    // Update the lights... (!!!!!! THE NUMBER OF LIGHTS HERE CANNOT CHANGE AT RUNTIME YET !!!!!
    // Need to recreate the uniform descriptors!)
    for (size_t i = 0; i < positionalLights_.size(); i++) {
        auto& uboLight = data.positionalLightData[i];
        // set data (TODO: this is terrible)
        positionalLights_[i].getLightData(uboLight);
        uboLight.position = camera.getCameraSpacePosition(uboLight.position);
    }
    for (size_t i = 0; i < spotLights_.size(); i++) {
        auto& uboLight = data.spotLightData[i];
        // set data (TODO: this is terrible)
        spotLights_[i].getLightData(data.spotLightData[i]);
        uboLight.position = camera.getCameraSpacePosition(uboLight.position);
        uboLight.direction = camera.getCameraSpaceDirection(uboLight.direction);
    }

    copy(dev);
}

void Uniform::Default::copy(const VkDevice& dev) {
    uint8_t* pData;
    mapResources(dev, pData);

    VkDeviceSize size = 0, offset = 0;

    // CAMERA
    size = sizeof(Camera::Data);
    memcpy(pData, data.pCamera, size);
    offset += size;
    // SHADER
    size = sizeof(Uniform::Default::DATA::ShaderData);
    memcpy(pData + offset, &data.shaderData.flags, size);
    offset += size;

    // LIGHTS
    // positional
    size = sizeof(Light::PositionalData) * data.positionalLightData.size();
    if (size) memcpy(pData + offset, data.positionalLightData.data(), size);
    offset += size;
    // spot
    size = sizeof(Light::SpotData) * data.spotLightData.size();
    if (size) memcpy(pData + offset, data.spotLightData.data(), size);
    offset += size;

    unmapResources(dev);
}

VkDescriptorSetLayoutBinding Uniform::Default::getDecriptorLayoutBinding(uint32_t binding, uint32_t count) const {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = nullptr;  // Optional
    return layoutBinding;
}

VkWriteDescriptorSet Uniform::Default::getWrite() {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_UNIFORM);
    write.dstArrayElement = ARRAY_ELEMENT;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &getBufferInfo();
    write.descriptorCount = 1;
    return write;
}

VkCopyDescriptorSet Uniform::Default::getCopy() {
    /*  1/9/19 I am not using this code because I think that this is only really necessary
        if you want to reuse the writes but with different bindings... I am currently not
        to sure about any of this.
    */
    VkCopyDescriptorSet copy = {};
    copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy.srcBinding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_UNIFORM);
    copy.srcArrayElement = ARRAY_ELEMENT;
    copy.dstBinding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_UNIFORM);
    copy.dstArrayElement = ARRAY_ELEMENT;
    return copy;
}

// **********************
//      Default Dynamic
// **********************

void Uniform::DefaultDynamic::init(const Shell::Context& ctx, const Game::Settings& settings, uint32_t count) {
    const auto& limits = ctx.physical_dev_props[ctx.physical_dev_index].properties.limits;

    VkDeviceSize size = sizeof(Texture::Data::flags);

    if (limits.minUniformBufferOffsetAlignment)
        size = (size + limits.minUniformBufferOffsetAlignment - 1) & ~(limits.minUniformBufferOffsetAlignment - 1);

    Base::init(ctx.dev, settings, size, static_cast<uint32_t>(textureFlags_.size()));
}

void Uniform::DefaultDynamic::update(const VkDevice& dev) {
    uint8_t* pData;
    mapResources(dev, pData);

    size_t offset = 0;
    for (uint32_t i = 0; i < textureFlags_.size(); i++) {
        textureFlags_[i] = Texture::FLAG::NONE;
        if (const auto pTexture = TextureHandler::getTexture(i)) {
            textureFlags_[i] = pTexture->flags;
        }
        memcpy(pData + offset, &textureFlags_[i], getInfo()->range);
        offset += getInfo()->range;
    }

    unmapResources(dev);
}

VkDescriptorSetLayoutBinding Uniform::DefaultDynamic::getDecriptorLayoutBinding(uint32_t binding, uint32_t count) const {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = nullptr;  // Optional
    return layoutBinding;
}

VkWriteDescriptorSet Uniform::DefaultDynamic::getWrite() {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM);
    write.dstArrayElement = ARRAY_ELEMENT;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write.pBufferInfo = &getBufferInfo();
    write.descriptorCount = 1;
    return write;
}

VkCopyDescriptorSet Uniform::DefaultDynamic::getCopy() {
    /*  1/9/19 I am not using this code because I think that this is only really necessary
        if you want to reuse the writes but with different bindings... I am currently not
        to sure about any of this.
    */
    VkCopyDescriptorSet copy = {};
    copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy.srcBinding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM);
    copy.srcArrayElement = ARRAY_ELEMENT;
    copy.dstBinding = static_cast<uint32_t>(DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM);
    copy.dstArrayElement = ARRAY_ELEMENT;
    return copy;
}
