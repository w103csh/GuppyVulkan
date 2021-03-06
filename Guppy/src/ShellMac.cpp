/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include <chrono>
#include <thread>
#include <vulkan/vulkan.h>

#include <Common/Helpers.h>

#include "Constants.h"
#include "Game.h"
#include "ShellMac.h"
// HANLDERS
#include "InputHandler.h"
#include "SoundHandler.h"
#include "SoundFModHandler.h"

namespace {

static std::function<void(std::string)> watchCallback;

void streamCallback(ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths,
                    const FSEventStreamEventFlags* eventFlags, const FSEventStreamEventId* eventIds) {
    char** pPaths = (char**)eventPaths;
    // printf("Callback called\n");
    for (int i = 0; i < numEvents; i++) {
        /* flags are unsigned long, IDs are uint64_t */
        // printf("Change %llu in %s, flags %lu\n", eventIds[i], paths[i], eventFlags[i]);
        auto filename = helpers::getFileName(pPaths[i]);
        watchCallback(filename);
    }
}

}  // namespace

ShellMac::ShellMac(Game& game)
    : Shell(game,
            Shell::Handlers{
                std::make_unique<Input::Handler>(this),
#ifdef USE_FMOD
                std::make_unique<Sound::FModHandler>(this),
#else
                std::make_unique<Sound::Handler>(this),
#endif
            }),
      watchDirStreamRef(NULL) {
}

ShellMac::~ShellMac() { cleanup(); }

void ShellMac::setPlatformSpecificExtensions() {
    // instanceExtensions_.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
}

void ShellMac::createWindow() { assert(false); }

PFN_vkGetInstanceProcAddr ShellMac::load() {
    assert(false);
    return nullptr;
}

vk::Bool32 ShellMac::canPresent(vk::PhysicalDevice phy, uint32_t queueFamily) {
    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(phy, queueFamily, context().surface, &supported);
    return supported;
}

void ShellMac::quit() const { assert(false); }

void ShellMac::destroyContext() {
    Shell::destroyContext();
    if (watchDirStreamRef) {
        FSEventStreamStop(watchDirStreamRef);
        FSEventStreamUnscheduleFromRunLoop(watchDirStreamRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        FSEventStreamInvalidate(watchDirStreamRef);
    }
}

void ShellMac::asyncAlert(uint64_t milliseconds) { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); };

void ShellMac::run() { assert(false); }

void ShellMac::watchDirectory(const std::string& directory, std::function<void(std::string)> callback) {
    if (settings_.enableDirectoryListener) {
        watchCallback = callback;
        auto fullPath = ROOT_PATH + directory;

        /* Define variables and create a CFArray object containing
         CFString objects containing paths to watch.
         */
        CFStringRef fullPathStrRef = CFStringCreateWithCString(NULL, fullPath.c_str(), kCFStringEncodingUTF8);
        CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void**)&fullPathStrRef, 1, NULL);
        FSEventStreamContext* context = NULL;  // could put stream-specific data here.
        CFAbsoluteTime latency = 0.0;          /* Latency in seconds */

        /* Create the stream, passing in a callback */
        watchDirStreamRef = FSEventStreamCreate(NULL,                              //
                                                &streamCallback,                   //
                                                context,                           //
                                                pathsToWatch,                      //
                                                kFSEventStreamEventIdSinceNow,     /* Or a previous event ID */
                                                latency,                           //
                                                kFSEventStreamCreateFlagFileEvents /* Flags explained in reference */
        );
        assert(watchDirStreamRef);

        // TODO: Create a thread dedicated to this?
        FSEventStreamScheduleWithRunLoop(watchDirStreamRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        assert(FSEventStreamStart(watchDirStreamRef));
    }
}
