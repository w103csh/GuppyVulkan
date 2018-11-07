#ifndef SHADER_H
#define SHADER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Camera.h"
#include "Light.h"
#include "MyShell.h"

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
           std::string markerName);

    enum class TYPE { COLOR_VERT = 0, TEX_VERT, COLOR_FRAG, LINE_FRAG, TEX_FRAG, UTIL_FRAG };

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

   private:
    std::string text_;
};

// **********************
//      ShaderHandler
// **********************

class ShaderHandler {
   public:
    static void init(MyShell &sh, const Game::Settings &settings, uint32_t numPosLights);
    static inline void destroy() { inst_.reset(); }

    ShaderHandler(const ShaderHandler &) = delete;             // Prevent construction by copying
    ShaderHandler &operator=(const ShaderHandler &) = delete;  // Prevent assignment

    static inline const Shader &getShader(Shader::TYPE type) { return inst_.shaders_[static_cast<int>(type)]; }
    static void posLightReplace(uint32_t numLights, std::string &text);
    static void recompileShader(std::string);

   private:
    ShaderHandler();  // Prevent construction

    static ShaderHandler inst_;

    void reset();
    void createShaderModules(uint32_t numPositionLights);
    template <typename T1, typename T2>
    static std::string textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2);

    MyShell::Context ctx_;     // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    std::vector<Shader> shaders_;
    // link shaders
    std::string utilFragText_;
};

#endif  // !SHADER_H