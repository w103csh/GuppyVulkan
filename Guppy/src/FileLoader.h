#ifndef FILELOADER_H
#define FILELOADER_H

#include <fstream>
#include <sstream>
#include <vector>

#include "Constants.h"

namespace FileLoader {

std::string readFile(std::string filepath) {
    std::ifstream file(ROOT_PATH + filepath, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string str = ss.str();

    file.close();

    return str;
}

};  // namespace FileLoader

#endif  // !FILELOADER_H