REM vertex
%VULKAN_SDK%\Bin\glslangValidator.exe -V .\color.vert
%VULKAN_SDK%\Bin\glslangValidator.exe -V .\texture.vert
REM fragment
%VULKAN_SDK%\Bin\glslangValidator.exe -V .\color.frag
%VULKAN_SDK%\Bin\glslangValidator.exe -V .\line.frag
%VULKAN_SDK%\Bin\glslangValidator.exe -V .\texture.fragment
