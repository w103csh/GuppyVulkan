/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Context.h"

#include "Helpers.h"

// See: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
static_assert(VK_HEADER_VERSION >= 126, "Vulkan version need 1.1.126.0 or greater");

// This cannot be a default compiler generated constructor. There was a very cryptic compiler error that happened after VS
// 2019 update to 16.5. I am too lazy to pin down the compiler version. I waited for MS to fix the compiler error so that it
// would give an actual error message but at this point I don't think its ever going to happen. Finding this was a nightmare.
Context::Context()
    : samplerAnisotropyEnabled(false),
      sampleRateShadingEnabled(false),
      linearBlittingSupported(false),
      computeShadingEnabled(false),
      tessellationShadingEnabled(false),
      geometryShadingEnabled(false),
      wireframeShadingEnabled(false),
      vertexAttributeDivisorEnabled(false),
      transformFeedbackEnabled(),
      debugMarkersEnabled(false),
      independentBlendEnabled(false),
      imageCubeArrayEnabled(false),
      instance{},
      physicalDev{},
      physicalDevIndex(0),
      memProps{},
      graphicsIndex(0),
      presentIndex(0),
      transferIndex(0),
      computeIndex(0),
      dev{},
      pAllocator(nullptr),
      surfaceProps{},
      surfaceFormat{},
      surface{},
      mode{},
      samples{},
      imageCount(0),
      extent{},
      normalizedScreenSpace{},
      depthFormat{},
      swapchain{},
      acquiredBackBuffer{},
      waitDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput),
      debugUtilsMessenger_{},
      dbg_() {}

void Context::initInstance(const char *appName, const char *engineName, const bool validate) {
    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Shell";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = vk::enumerateInstanceVersion();

    vk::InstanceCreateInfo instInfo = {};
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = static_cast<uint32_t>(instanceEnabledLayerNames.size());
    instInfo.ppEnabledLayerNames = instanceEnabledLayerNames.data();
    instInfo.enabledExtensionCount = static_cast<uint32_t>(instanceEnabledExtensionNames.size());
    instInfo.ppEnabledExtensionNames = instanceEnabledExtensionNames.data();

    instance = vk::createInstance(instInfo, pAllocator);
    assert(instance);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    // Moved asserts below from old Extensions.h. Not sure yet if there is a better place.

    if (validate) {
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateDebugUtilsMessengerEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyDebugUtilsMessengerEXT);
    }
}

void Context::destroyInstance() { instance.destroy(pAllocator); }

void Context::initDevice() {
    // queue create info
    std::set<uint32_t> unique_queue_families = {
        graphicsIndex,
        presentIndex,
        transferIndex,
        computeIndex,
    };
    float queue_priorities[1] = {0.0f};  // only one queue per family atm
    std::vector<vk::DeviceQueueCreateInfo> queue_infos;
    for (auto &index : unique_queue_families) {
        vk::DeviceQueueCreateInfo queueInfo = {};
        queueInfo.queueCount = 1;  // only one queue per family atm
        queueInfo.queueFamilyIndex = index;
        queueInfo.pQueuePriorities = queue_priorities;
        queue_infos.push_back(queueInfo);
    }

    // features
    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = samplerAnisotropyEnabled;
    deviceFeatures.sampleRateShading = sampleRateShadingEnabled;
    deviceFeatures.fragmentStoresAndAtomics = computeShadingEnabled;
    deviceFeatures.tessellationShader = tessellationShadingEnabled;
    deviceFeatures.geometryShader = geometryShadingEnabled;
    deviceFeatures.fillModeNonSolid = wireframeShadingEnabled;
    deviceFeatures.independentBlend = independentBlendEnabled;
    deviceFeatures.imageCubeArray = imageCubeArrayEnabled;

    // Phyiscal device extensions names
    auto &phyDevProps = physicalDevProps[physicalDevIndex];
    std::vector<const char *> enabledExtensionNames;
    for (const auto &extInfo : phyDevProps.phyDevExtInfos)
        if (extInfo.valid) enabledExtensionNames.push_back(extInfo.name);

    vk::DeviceCreateInfo devInfo = {};
    devInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    devInfo.pQueueCreateInfos = queue_infos.data();
    devInfo.pEnabledFeatures = &deviceFeatures;
    devInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());
    devInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

    // Physical device extension featrues (For some reason using the name above doesn't always work)
    // void **pNext = &(const_cast<void *>(devInfo.pNext));
    if (vertexAttributeDivisorEnabled) {
        assert(false);  // Never tested. Was using the name above sufficient?
        // *pNext = &phyDevProps.featVertAttrDiv;
        // pNext = &phyDevProps.featVertAttrDiv.pNext;
    }
    if (transformFeedbackEnabled) {
        assert(false);
        // *pNext = &phyDevProps.featTransFback;
        // pNext = &phyDevProps.featTransFback.pNext;
    }

    dev = physicalDev.createDevice(devInfo, pAllocator);
    assert(dev);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(dev);

    // Moved asserts below from old Extensions.h. Not sure yet if there is a better place.

    if (debugMarkersEnabled) {
        // There are more of these and really there needs to be a more robust solution. As of now there is no way to enable
        // these.
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectTagEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectNameEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerBeginEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerEndEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerInsertEXT);

        dbg_.init(dev, true);
    }
    if (transformFeedbackEnabled) {
        assert(false);  // You probably don't want to use transform feedback see below.
        /**
         * I started trying to implement a simple a vertex update on the gpu using transform feedback while following along
         * with the shader book I'm reading. As I was doing this it was becoming increasingly clear that this was going to be
         * difficult due to the lack of working examples I could find online. That is when I stumbled upon this blog post by
         * a Khronos employee: http://jason-blog.jlekstrand.net/2018/10/transform-feedback-is-terrible-so-why.html. After
         * reading this I will take his adivce and implement the update on the gpu using compute shaders, but I am going to
         * leave this code here because I liked the changes I made to the extensions code in this file, and the
         * Shell/Settings code elsewhere. Also, it should be a good reminder to not just following things blindly.
         */
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdBindTransformFeedbackBuffersEXT);
    }
}

void Context::destroyDevice() {
    dev.waitIdle();
    dev.destroy(pAllocator);
}

void Context::initDebug(const bool validate, const bool validateVerbose,
                        const PFN_vkDebugUtilsMessengerCallbackEXT pCallback, void *pUserData) {
    if (!validate) return;

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

    if (validateVerbose) {
        debugInfo.messageSeverity |=
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
        debugInfo.messageType |=
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    }

    debugInfo.pfnUserCallback = pCallback;
    debugInfo.pUserData = pUserData;

    debugUtilsMessenger_ = instance.createDebugUtilsMessengerEXT(debugInfo, pAllocator);
}

void Context::destroyDebug() {
    if (debugUtilsMessenger_) instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger_, pAllocator);
}

void Context::createBuffer(const vk::CommandBuffer &cmd, const vk::BufferUsageFlags usage, const vk::DeviceSize size,
                           const std::string &&name, BufferResource &stgRes, BufferResource &buffRes, const void *data,
                           const bool mappable) const {
    assert(!stgRes.buffer);
    assert(!stgRes.memory);
    assert(!buffRes.buffer);
    assert(!buffRes.memory);

    // STAGING RESOURCE
    buffRes.memoryRequirements.size =
        helpers::createBuffer(dev, size, vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, memProps,
                              stgRes.buffer, stgRes.memory, pAllocator);

    // FILL STAGING BUFFER ON DEVICE
    void *pData;
    pData = dev.mapMemory(stgRes.memory, 0, buffRes.memoryRequirements.size);
    /*
        You can now simply memcpy the vertex data to the mapped memory and unmap it again using unmapMemory.
        Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
        of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
        are two ways to deal with that problem:
            - Use a memory heap that is host coherent, indicated with vk::MemoryPropertyFlagBits::eHostCoherent
            - Call flushMappedMemoryRanges to after writing to the mapped memory, and call
              invalidateMappedMemoryRanges before reading from the mapped memory
        We went for the first approach, which ensures that the mapped memory always matches the contents of the
        allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
        but we'll see why that doesn't matter in the next chapter.
    */
    memcpy(pData, data, static_cast<size_t>(size));
    dev.unmapMemory(stgRes.memory);

    // FAST VERTEX BUFFER
    vk::MemoryPropertyFlags memPropFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
    if (mappable) memPropFlags |= vk::MemoryPropertyFlagBits::eHostVisible;
    helpers::createBuffer(dev, size,
                          // TODO: probably don't need to check memory requirements again
                          usage, memPropFlags, memProps, buffRes.buffer, buffRes.memory, pAllocator);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stgRes.buffer, buffRes.buffer, buffRes.memoryRequirements.size);
    dbg_.setMarkerName(buffRes.buffer, name.c_str());
}

void Context::destroyBuffer(BufferResource &res) const {
    if (res.buffer) dev.destroyBuffer(res.buffer, pAllocator);
    if (res.memory) dev.freeMemory(res.memory, pAllocator);
}
