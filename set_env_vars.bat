:: This script needs to be ran as administrator on windows.
@ECHO OFF

:: Set these to right location when setting up environment
SETX GLFW_INCLUDE_DIR "C:/libs/glfw-3.3.2.bin.WIN64/include" /M
SETX GLFW_LIB "C:/libs/glfw-3.3.2.bin.WIN64/lib-vc2019/glfw3.lib" /M
SETX GLM_LIB_DIR "C:/libs/glm-0.9.9.7/glm" /M
SETX FMOD_DIR "C:/libs/FMOD SoundSystem/FMOD Studio API Windows" /M
SETX IMGUI_REPO_DIR "C:/Users/cloin/source/repos/imgui" /M
:: To setup the glslang build directory correctly you should run cmake in the build directory as follows:
::      cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="./install" .. -A x64
::      cmake --build . --config Debug --target install
:: then,
::      cmake --build . --config Release --target install
:: This will allow you to build both Debug, and Release libraries in the same install directory.
::
:: NOTE: I just tried this again and it doesn't work on windows using msvc generator. I had to open the
::       generated solutions (skipped --build step), and build the INSTALL project for each config. Also,
::       the tests failed to build properly and there were a bunch of errors.
::
SETX GLSLANG_INSTALL_DIR "C:/Users/cloin/source/repos/glslang/build/install" /M
::SETX VULKAN_HEADERS_INSTALL_DIR "C:/Users/cloin/source/repos/Vulkan-Headers/build/install" /M
:: I would imagine that I would have to do something similar to GLSLANG if I ever want to compile
:: the loader into my projects. I don't think these are being used atm.
::SETX VULKAN_LOADER_INSTALL_DIR "C:/Users/cloin/source/repos/Vulkan-Loader/build/RelWithDebInfo/install" /M

PAUSE