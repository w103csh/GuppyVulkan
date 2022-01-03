/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef INCLUDER_H
#define INCLUDER_H

#include <string>
#include <vector>

#include "glslang/Public/ShaderLang.h"

class Includer : public glslang::TShader::Includer {
   public:
    IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override;

    // Just let the vector clean it up for now. The pattern doesn't make sense for the way I'm compiling shaders atm anyway.
    // This should be changed if I ever care about compiling the shaders fast.
    void releaseInclude(IncludeResult*) override {}

   private:
    std::vector<std::string> headers;
    std::vector<IncludeResult> results;
};

#endif  // !INCLUDER_H