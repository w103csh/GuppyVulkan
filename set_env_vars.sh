# I had permission issues sourcing this script and then running buildMac.py.
# Leaving this as another breadcrumb for what variables need to be set.

# Vulkan
export VULKAN_SDK=/Users/shared/libs/vulkansdk-macos-1.1.101.0/macOS # this is also necessary!
export VK_LAYER_PATH=$VULKAN_SDK/etc/vulkan/explicit_layer.d
export VK_ICD_FILENAMES=$VULKAN_SDK/etc/vulkan/icd.d/MoltenVK_icd.json
# MoltenVK
export MVK_PACKAGE_DIR=/Users/colin/Repos/MoltenVK/Package
# GLM
export GLM_LIB_DIR=/Users/Shared/libs/glm-0.9.9.5/glm
# GLFW
export GLFW_INCLUDE_DIR=/Users/colin/Repos/glfw/include
export GLFW_LIB=/Users/colin/Repos/glfw/glfw-build/src/libglfw.3.dylib
