#ifndef FILELOADER_H
#define FILELOADER_H

#include <memory>
#include <string>

class Mesh;

namespace FileLoader {

std::string readFile(std::string filepath);

void loadObj(Mesh *pMesh);

};  // namespace FileLoader

#endif  // !FILELOADER_H