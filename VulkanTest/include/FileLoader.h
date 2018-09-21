
#pragma once

#include <vector>

class FileLoader
{
public:
    static std::vector<char> readFile(const std::string& filename);
};