//
//#ifndef STAG_BUFF_HANDLER_H
//#define STAG_BUFF_HANDLER_H
//
//#include <mutex>
//#include <vector>
//#include <vulkan/vulkan.h>
//
//#include "Helpers.h"
//#include "MyShell.h"
//
// constexpr auto NUM_STAG_BUFF_FENCES = 2;
//
// struct StagingBufferResource {
//    VkBuffer buffer;
//    VkDeviceMemory memory;
//};
//
// class StagingBufferHandler {
//   public:
//    static StagingBufferHandler &get(const MyShell *sh = nullptr, const VkDevice &dev = nullptr,
//                                     const CommandData *cmd_data = nullptr) {
//        static StagingBufferHandler instance(sh, dev, cmd_data);
//        return instance;
//    };
//
//    StagingBufferHandler() = delete;                                         // Default construct
//    StagingBufferHandler(StagingBufferHandler const &) = delete;             // Copy construct
//    StagingBufferHandler(StagingBufferHandler &&) = delete;                  // Move construct
//    StagingBufferHandler &operator=(StagingBufferHandler const &) = delete;  // Copy assign
//    StagingBufferHandler &operator=(StagingBufferHandler &&) = delete;       // Move assign
//
//    enum class BEGIN_TYPE { ALLOC, RESET };
//    enum class END_TYPE { DESTROY, RESET };
//
//    void begin_command_recording(VkCommandBuffer &cmd, BEGIN_TYPE type = BEGIN_TYPE::ALLOC,
//                                 VkCommandBufferResetFlags resetflags = 0);
//
//    void end_recording_and_submit(StagingBufferResource *resources, size_t num_resources, VkCommandBuffer &cmd,
//                                  uint32_t wait_queue_family = UINT32_MAX, VkCommandBuffer *wait_cmd = nullptr,
//                                  VkPipelineStageFlags *wait_stages = 0, END_TYPE type = END_TYPE::DESTROY);
//
//   private:
//    StagingBufferHandler(const MyShell *sh, const VkDevice &dev, const CommandData *cmd_data);
//
//    struct CommandResources {
//        size_t num_resources;
//        StagingBufferResource *resources;
//        VkSemaphore semaphore;
//        std::array<VkFence, NUM_STAG_BUFF_FENCES> fences;
//        uint32_t wait_queue_family;
//        std::array<VkCommandBuffer, NUM_STAG_BUFF_FENCES> cmds;
//    };
//
//    void wait(CommandResources *cmd_res, END_TYPE type);
//
//    const MyShell *sh_;
//    VkDevice dev_;
//    const CommandData *cmd_data_;
//    std::mutex mutex_;
//};
//
//#endif  // !STAG_BUFF_HANDLER_H
