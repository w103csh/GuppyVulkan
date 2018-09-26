
#include <fstream>
#include <sstream>

#include "FileLoader.h"

const std::string FileLoader::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string str = ss.str();

    file.close();

    return str;
}