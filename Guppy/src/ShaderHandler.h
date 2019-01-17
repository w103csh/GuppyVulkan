#ifndef SHADER_HANDLER_H
#define SHADER_HANDLER_H

#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Singleton.h"
#include "Shader.h"
#include "Shell.h"
#include "Texture.h"
#include "UniformBuffer.h"

class Camera;
class Scene;

namespace Shader {

// **********************
//      Handler
// **********************

class Handler : public Singleton {
   public:
    static void init(Shell &sh, const Game::Settings &settings, const Camera &camera);
    static inline void destroy() { inst_.reset(); }

    Handler(const Handler &) = delete;             // Prevent construction by copying
    Handler &operator=(const Handler &) = delete;  // Prevent assignment

    static std::unique_ptr<Shader::Base> &getShader(SHADER_TYPE &&type);
    static std::unique_ptr<Shader::Base> &getShader(const SHADER_TYPE &type);

    static void getStagesInfo(std::vector<SHADER_TYPE> &&types, std::vector<VkPipelineShaderStageCreateInfo> &stagesInfo);

    static void update(std::unique_ptr<Scene> &pScene);
    static void lightMacroReplace(std::string &&macroVariable, uint16_t &&numLights, std::string &text);
    static void recompileShader(std::string);
    static void appendLinkTexts(Shader::Base *pShader, std::vector<const char *> &texts);

    // DESCRIPTOR
    static void updateDescriptorSets(const std::set<DESCRIPTOR_TYPE> types, std::vector<VkDescriptorSet> &sets,
                                     uint32_t setCount, const std::shared_ptr<Texture::Data> &pTexture = nullptr);
    static std::vector<uint32_t> getDynamicOffsets(const std::set<DESCRIPTOR_TYPE> types);

    static void cleanup();

   private:
    Handler();     // Prevent construction
    ~Handler(){};  // Prevent construction
    static Handler inst_;
    void reset() override;

    bool loadShaders(std::vector<SHADER_TYPE> types, bool doAssert, bool load = true);

    template <typename T1, typename T2>
    static std::string textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2);

    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    // SHADERS
    std::unique_ptr<Shader::Base> pDefColorVertShader_;
    std::unique_ptr<Shader::Base> pDefColorFragShader_;
    std::unique_ptr<Shader::Base> pDefLineFragShader_;
    std::unique_ptr<Shader::Base> pDefTexVertShader_;
    std::unique_ptr<Shader::Base> pDefTexFragShader_;
    // link
    std::unique_ptr<Shader::Base> pUtilFragShader_;

    std::queue<PIPELINE_TYPE> updateQueue_;
    std::vector<VkShaderModule> oldModules_;

    // **********************
    //      UNIFORM BUFFER
    // **********************
   public:
    static void updateDefaultUniform(Camera &camera);
    static void updateDefaultDynamicUniform();

    // DESCRIPTOR
    static VkDescriptorSetLayoutBinding getDescriptorLayoutBinding(const DESCRIPTOR_TYPE &type);
    static void getDescriptorSetTypes(const SHADER_TYPE &type, std::set<DESCRIPTOR_TYPE> &set) {
        inst_.getShader(type)->getDescriptorTypes(set);
    }

    // TODO: These names are terrible. Make a template type condition function.
    static uint16_t getDefUniformPositionLightCount() { return inst_.pDefaultUniform_->getPositionLightCount(); }
    static uint16_t getDefUniformSpotLightCount() { return inst_.pDefaultUniform_->getSpotLightCount(); }

    template <typename T>
    static void defaultLightsAction(T &&updateFunc) {
        inst_.pDefaultUniform_->lightsAction(std::forward<T &&>(updateFunc));
    }

    template <typename T>
    static void defaultUniformAction(T &&updateFunc) {
        inst_.pDefaultUniform_->uniformAction(std::forward<T &&>(updateFunc));
    }

   private:
    std::unique_ptr<Uniform::Default> pDefaultUniform_;
    std::unique_ptr<Uniform::DefaultDynamic> pDefaultDynamicUniform_;
};

}  // namespace Shader

#endif  // !SHADER_HANDLER_H