#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <vector>
#include <future>

#include "Singleton.h"

class Mesh;
class Model;
class MyShell;

class ModelHandler : Singleton {
   public:
    static void init(MyShell* sh);
    static inline void destroy() { inst_.reset(); }

    static void loadModel(std::function<void(MyShell*)> ldgFunction, bool async);
    static void update();

   private:
    ModelHandler() : sh_(nullptr){};  // Prevent construction
    ~ModelHandler(){};                // Prevent construction
    static ModelHandler inst_;
    void reset() override{};

    MyShell* sh_;  // TODO: shared_ptr

    std::vector<std::future<void>> ldgFutures_;
};

#endif  // !MODEL_HANDLER_H
