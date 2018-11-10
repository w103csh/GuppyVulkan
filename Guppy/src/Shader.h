#ifndef SHADER_H
#define SHADER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Camera.h"
#include "Light.h"
#include "MyShell.h"


enum class SHADER_TYPE { COLOR_VERT = 0, TEX_VERT, COLOR_FRAG, LINE_FRAG, TEX_FRAG, UTIL_FRAG };

struct DefaultUniformBuffer {
    Camera::Data camera;
    std::vector<Light::Positional::Data> positionalLights;
};

// **********************
//      Shader
// **********************

class Shader {
   public:
    Shader(const VkDevice dev, std::string fileName, std::vector<const char *> pLinkTexts, VkShaderStageFlagBits stage,
           bool updateTextFromFile = true, std::string markerName = "");
    ~Shader(){};

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

    void init(const VkDevice dev, std::vector<const char *> pLinkTexts, bool doAssert, std::vector<VkShaderModule> &oldModules,
              bool updateTextFromFile = true);

   private:
    std::string fileName_;
    std::string markerName_;
    std::string text_;
    VkShaderStageFlagBits stage_;
};

// **********************
//      ShaderHandler
// **********************

class Scene;

class ShaderHandler {
   public:
    static void init(MyShell &sh, const Game::Settings &settings, uint32_t numPosLights);
    static inline void destroy() { inst_.reset(); }

    ShaderHandler(const ShaderHandler &) = delete;             // Prevent construction by copying
    ShaderHandler &operator=(const ShaderHandler &) = delete;  // Prevent assignment

    static inline const std::unique_ptr<Shader> &getShader(SHADER_TYPE type) { return inst_.shaders_[static_cast<int>(type)]; }
    static inline void setNumPosLights(uint32_t numPosLights) { inst_.numPosLights_ = numPosLights; }

    static bool update(std::unique_ptr<Scene> &pScene);
    static void posLightReplace(uint32_t numLights, std::string &text);
    static void recompileShader(std::string);
    static void cleanupOldResources();

   private:
    ShaderHandler();  // Prevent construction

    static ShaderHandler inst_;

    void reset();
    bool loadShaders(std::vector<SHADER_TYPE> types, bool doAssert, bool updateFromFile = true);
    void loadLinkText(SHADER_TYPE type, bool updateFromFile = true);

    template <typename T1, typename T2>
    static std::string textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2);

    MyShell::Context ctx_;     // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    std::vector<std::unique_ptr<Shader>> shaders_;
    uint32_t numPosLights_;
    // link shaders
    std::string utilFragText_;

    std::queue<PIPELINE_TYPE> updateQueue_;
    std::vector<VkShaderModule> oldModules_;
};

#endif  // !SHADER_H