#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <glm/glm.hpp>
#include <map>
#include <tiny_obj_loader.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#define USE_DEBUG_UI
#define DO_PROJECTOR 0  // Not sure where this should go, or if this is a good solution to the problem.

// Class functions with a virtual base cannot have constexpr member functions
// with clang. This makes me think that msvc is just ignoring it, but I'll leave
// this for now.
#ifdef __clang__
#define virtual_inline inline
#elif _MSC_VER
#define virtual_inline constexpr
#endif

constexpr bool PRIMARY_MONITOR = 0;
// TODO: a proper dynamic buffer allocator.
/*  I am using this for the mean time to restrict the number of textures, so
    that I don't have to write an allocator before I know what is necessary. The limit
    is for the dynamic offest uniform that has texture flags. The flags indicate the
    number of sampler array layers. Setting a limit let me just create the dynamic
    uniform buffer size out of the gate, and then refer to offsets without ... an
    allocator.
*/
constexpr uint32_t TEXTURE_LIMIT = 30;
constexpr auto APP_SHORT_NAME = "Guppy";
constexpr VkExtent2D BAD_EXTENT_2D = {UINT32_MAX, UINT32_MAX};
constexpr VkExtent3D BAD_EXTENT_3D = {UINT32_MAX, UINT32_MAX, 1};
constexpr float BAD_ASPECT = FLT_MAX;

constexpr float T_MAX = 1.0f;

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

#define M_PI_FLT static_cast<float>(M_PI)      // pi
#define M_PI_2_FLT static_cast<float>(M_PI_2)  // pi/2
#define M_PI_4_FLT static_cast<float>(M_PI_4)  // pi/4

/* Number of viewports and number of scissors have to be the same */
/* at pipeline creation and in any call to set them dynamically   */
/* They also have to be the same as each other                    */
#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation",
};

const std::vector<const char*> INSTANCE_EXTENSIONS = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME, in init_swapchain_extension now
};

const glm::vec4 COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
const glm::vec4 COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
const glm::vec4 COLOR_YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};

// X
const glm::vec3 CARDINAL_X = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 CARDINAL_X_POS = glm::vec4(CARDINAL_X, 1.0f);
const glm::vec3 CARDINAL_X_DIR = glm::vec4(CARDINAL_X, 0.0f);
// Y
const glm::vec3 CARDINAL_Y = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 CARDINAL_Y_POS = glm::vec4(CARDINAL_Y, 1.0f);
const glm::vec3 CARDINAL_Y_DIR = glm::vec4(CARDINAL_Y, 0.0f);
// Z
const glm::vec3 CARDINAL_Z = glm::vec3(0.0f, 0.0f, 1.0f);
const glm::vec3 CARDINAL_Z_POS = glm::vec4(CARDINAL_Z, 1.0f);
const glm::vec3 CARDINAL_Z_DIR = glm::vec4(CARDINAL_Z, 0.0f);
// DIRECTIONAL
const glm::vec3 UP_VECTOR = CARDINAL_Y;
const glm::vec3 FORWARD_VECTOR = CARDINAL_Z;

const auto ENABLE_SAMPLE_SHADING = VK_TRUE;
const float MIN_SAMPLE_SHADING = 0.2f;

// ROOT
#if defined(VK_USE_PLATFORM_WIN32_KHR)
const std::string ROOT_PATH = "../../../";  // TODO: this should come from something meaningful ...
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
const std::string ROOT_PATH = "../../../../";  // TODO: this should come from something meaningful ...
#endif

// DATA
const std::string DATA_PATH = ROOT_PATH + "data/";

// MODELS
const std::string MODEL_PATH = DATA_PATH + "models/";
// chalet
const std::string CHALET_MODEL_PATH = MODEL_PATH + "chalet_obj/chalet.obj";
const std::string CHALET_TEX_PATH = MODEL_PATH + "chalet_obj/chalet.jpg";
// medieval house
const std::string MED_H_MODEL_PATH = MODEL_PATH + "Medieval_House_obj/Medieval_House.obj";
const std::string MED_H_DIFF_TEX_PATH = MODEL_PATH + "Medieval_House_obj/Medieval_House_Diff.png";
const std::string MED_H_NORM_TEX_PATH = MODEL_PATH + "Medieval_House_obj/Medieval_House_Nor.png";
const std::string MED_H_SPEC_TEX_PATH = MODEL_PATH + "Medieval_House_obj/Medieval_House_Spec.png";
// orange
const std::string ORANGE_MODEL_PATH = MODEL_PATH + "Orange_obj/Orange.obj";
const std::string ORANGE_DIFF_TEX_PATH = MODEL_PATH + "Orange_obj/Color.jpg";
const std::string ORANGE_NORM_TEX_PATH = MODEL_PATH + "Orange_obj/Normal.jpg";
// grass
const std::string GRASS_LP_MODEL_PATH = MODEL_PATH + "grass (low-poly)/grass_low_poly.obj";
const std::string GRASS_LP_DIFF_TEX_PATH = MODEL_PATH + "grass (low-poly)/low_poly_grass_2.jpg";  // not diffuse apparently
const std::string GRASS_LP_NORM_TEX_PATH = MODEL_PATH + "grass (low-poly)/low_poly_grass_normal.jpg";
const std::string GRASS_LP_SPEC_TEX_PATH = MODEL_PATH + "grass (low-poly)/low_poly_grass_specular.jpg";
const std::string GRASS_LP_OPAC_TEX_PATH = MODEL_PATH + "grass (low-poly)/low_poly_grass_opacity.jpg";
// pear
const std::string PEAR_MODEL_PATH = MODEL_PATH + "pear_export_obj/pear_export.obj";
// general
const std::string SPHERE_MODEL_PATH = MODEL_PATH + "sphere.obj";
const std::string ICOSAHEDRON_MODEL_PATH = MODEL_PATH + "icosahedron.obj";
const std::string TORUS_MODEL_PATH = MODEL_PATH + "torus.obj";
const std::string PIG_MODEL_PATH = MODEL_PATH + "pig_triangulated.obj";
const std::string DRAGON_MODEL_PATH = MODEL_PATH + "dragon.obj";

#endif  // !CONSTANTS_H
