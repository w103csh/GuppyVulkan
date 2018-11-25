
#include "FileLoader.h"
#include "Model.h"
#include "ModelHandler.h"

ModelHandler ModelHandler::inst_;

void ModelHandler::init(MyShell* sh) { inst_.sh_ = sh; }

void ModelHandler::loadModel(std::function<void(MyShell*)> ldgFunction, bool async) {
    if (async) {
        auto fut = std::async(std::launch::async, ldgFunction, inst_.sh_);
        inst_.ldgFutures_.emplace_back(std::move(fut));
    } else {
        ldgFunction(inst_.sh_);
    }
}

void ModelHandler::update() {
    // Check mesh futures
    if (!inst_.ldgFutures_.empty()) {
        auto itFut = inst_.ldgFutures_.begin();

        while (itFut != inst_.ldgFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                // Finish future
                fut.get();
                // Remove the future from the list if all goes well.
                itFut = inst_.ldgFutures_.erase(itFut);

            } else {
                ++itFut;
            }
        }
    }
}
