
#include "Game.h"

#include "Helpers.h"
#include "Types.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"
#include "UniformHandler.h"

Game::~Game() = default;

Game::Game(const std::string& name, const std::vector<std::string>& args, Handlers&& handlers)
    : handlers_(std::move(handlers)), settings_(), shell_(nullptr) {
    settings_.name = name;
    parse_args(args);
}

void Game::watchDirectory(const std::string& directory, std::function<void(std::string)> callback) {
    shell_->watchDirectory(directory, callback);
}

Game::Settings::Settings()
    : name(""),
      initial_width(1920),
      initial_height(1080),
      queue_count(1),
      back_buffer_count(3),
      ticks_per_second(30),
      vsync(true),
      animate(true),
      validate(true),
      validate_verbose(false),
      no_tick(false),
      no_render(false),
      try_sampler_anisotropy(true),  // TODO: Not sure what this does
      try_sample_rate_shading(true),
      try_compute_shading(true),
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
      try_tessellation_shading(false),
      try_geometry_shading(false),
#else
      try_tessellation_shading(true),
      try_geometry_shading(true),
#endif
      try_wireframe_shading(true),
      try_debug_markers(false),
      try_independent_blend(true),
      enable_sample_shading(true),
      enable_double_clicks(false),
      enable_directory_listener(true),
      assert_on_recompile_shader(false) {
}

void Game::Handler::createBuffer(const VkCommandBuffer& cmd, const VkBufferUsageFlagBits usage, const VkDeviceSize size,
                                 const std::string&& name, BufferResource& stgRes, BufferResource& buffRes, const void* data,
                                 const bool mappable) {
    const auto& ctx = shell().context();

    // STAGING RESOURCE
    buffRes.memoryRequirements.size =
        helpers::createBuffer(ctx.dev, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ctx.memProps,
                              stgRes.buffer, stgRes.memory);

    // FILL STAGING BUFFER ON DEVICE
    void* pData;
    vk::assert_success(vkMapMemory(ctx.dev, stgRes.memory, 0, buffRes.memoryRequirements.size, 0, &pData));
    /*
        You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory.
        Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
        of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
        are two ways to deal with that problem:
            - Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            - Call vkFlushMappedMemoryRanges to after writing to the mapped memory, and call
              vkInvalidateMappedMemoryRanges before reading from the mapped memory
        We went for the first approach, which ensures that the mapped memory always matches the contents of the
        allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
        but we'll see why that doesn't matter in the next chapter.
    */
    memcpy(pData, data, static_cast<size_t>(size));
    vkUnmapMemory(ctx.dev, stgRes.memory);

    // FAST VERTEX BUFFER
    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (mappable) memProps |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    helpers::createBuffer(ctx.dev, size,
                          // TODO: probably don't need to check memory requirements again
                          usage, memProps, ctx.memProps, buffRes.buffer, buffRes.memory);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stgRes.buffer, buffRes.buffer, buffRes.memoryRequirements.size);

    // Name the buffers for debugging
    if (shell().context().debugMarkersEnabled) {
        std::string markerName = name + " buffer";
        ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)buffRes.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      markerName.c_str());
    }
}
