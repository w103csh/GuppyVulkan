#ifndef SHADER_H
#define SHADER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Camera.h"
#include "Constants.h"
#include "Light.h"
#include "Shell.h"
#include "Singleton.h"

// **********************
//      Shader
// **********************

class Shader {
   public:
    typedef enum FLAGS {
        DEFAULT = 0x00000000,
        // Should this be on the "Dynamic" uniform buffer? Now its on "Default". If it stays then
        // move it to Scene.
        TOON_SHADE = 0x00000001,
        // THROUGH 0x0000000F
        FOG_LINEAR = 0x00000010,
        FOG_EXP = 0x00000020,
        FOG_EXP2 = 0x00000040,
        // THROUGH 0x0000000F (used by shader)
        BITS_MAX_ENUM = 0x7FFFFFFF
    } FLAGS;

    Shader(const VkDevice dev, std::string fileName, std::vector<const char *> pLinkTexts, VkShaderStageFlagBits stage,
           bool updateTextFromFile = true, std::string markerName = "");
    ~Shader(){};

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

    void init(const VkDevice dev, std::vector<const char *> pLinkTexts, bool doAssert,
              std::vector<VkShaderModule> &oldModules, bool updateTextFromFile = true);

   private:
    std::string fileName_;
    std::string markerName_;
    std::string text_;
    VkShaderStageFlagBits stage_;
};

// **********************
//      Default Uniform
// **********************

// TODO: move uniform handling here...
struct DefaultUniformBuffer {
    const Camera::Data *pCamera;
    struct ShaderData {
        alignas(16) FlagBits flags = Shader::FLAGS::DEFAULT;
        // 12 rem
        struct Fog {
            float minDistance = 0.0f;
            float maxDistance = 40.0f;
            float density = 0.05f;
            alignas(4) float __padding1;                // rem 4
            alignas(16) glm::vec3 color = CLEAR_COLOR;  // rem 4
        } fog;
    } shaderData;
    std::vector<Light::PositionalData> positionalLightData;
    std::vector<Light::SpotData> spotLightData;
};

// **********************
//      ShaderHandler
// **********************

class Scene;

class ShaderHandler : Singleton {
   public:
    static void init(Shell &sh, const Game::Settings &settings, uint32_t numPosLights, uint32_t numSpotLights);
    static inline void destroy() { inst_.reset(); }

    ShaderHandler(const ShaderHandler &) = delete;             // Prevent construction by copying
    ShaderHandler &operator=(const ShaderHandler &) = delete;  // Prevent assignment

    static inline const std::unique_ptr<Shader> &getShader(SHADER_TYPE type) {
        return inst_.shaders_[static_cast<int>(type)];
    }
    static inline void setNumPosLights(uint32_t numPosLights) { inst_.numPosLights_ = numPosLights; }

    static void update(std::unique_ptr<Scene> &pScene);
    static void lightMacroReplace(std::string macroVariable, uint32_t numLights, std::string &text);
    static void recompileShader(std::string);
    static void cleanupOldResources();

   private:
    ShaderHandler();     // Prevent construction
    ~ShaderHandler(){};  // Prevent construction
    static ShaderHandler inst_;
    void reset() override;

    bool loadShaders(std::vector<SHADER_TYPE> types, bool doAssert, bool updateFromFile = true);
    void loadLinkText(SHADER_TYPE type, bool updateFromFile = true);

    template <typename T1, typename T2>
    static std::string textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2);

    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    std::vector<std::unique_ptr<Shader>> shaders_;
    uint32_t numPosLights_, numSpotLights_;
    // link shaders
    std::string utilFragText_;

    std::queue<PIPELINE_TYPE> updateQueue_;
    std::vector<VkShaderModule> oldModules_;
};

#endif  // !SHADER_H