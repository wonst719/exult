// This file is not part of SDL
// It just allows exult to #include <SDL_syswm.h>
// instead of <SDL2/SDL_syswm.h>
// without needing to mess with the include path from vcpkg by using
// $(_ZVcpkgCurrentInstalledDir)include\SDL2 I do not feel comfortable
// referencing an undocumented msbuild macro used by vcpkg. These header files
// may not be elegant but they work and don't rely on undocumented features and
// wont break if vcpkg updates
#pragma once
#include <SDL2\SDL_syswm.h>