/*
 * Modifications copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * -------------------------------
 * 
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTIL_H
#define UTIL_H

#include <vector>

#ifdef _WIN32
#pragma comment(linker, "/subsystem:console")

#ifndef NOMINMAX
#define NOMINMAX /* Don't let Windows define min() or max() */
#endif
#endif

#include <vulkan/vulkan.h>

bool GLSLtoSPV(const VkShaderStageFlagBits shaderType, std::vector<const char *> pShaders, std::vector<unsigned int> &spirv);
void init_glslang();
void finalize_glslang();

#endif  // !UTIL_H
