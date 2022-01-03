/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Includer.h"

#include "../FileLoader.h"
#include "../ShaderConstants.h"

using IncludeResult = glslang::TShader::Includer::IncludeResult;

IncludeResult* Includer::includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) {
    headers.push_back(FileLoader::readFile(Shader::BASE_DIRNAME + std::string(headerName)));
    results.emplace_back(headerName, headers.back().c_str(), headers.back().size(), nullptr);
    return &results.back();
}