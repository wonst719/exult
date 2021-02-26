#pragma once
#pragma warning(disable : 4996)
#define NOMINMAX

#define VERSION       "1.7.0git"
#define EXULT_DATADIR "data/"

// Don't need everything in the windows headers
#define WIN32_LEAN_AND_MEAN

// Disable some warnings
#pragma warning(disable : 4786)    // Debug Len > 255
#pragma warning( \
		disable : 4355)    // 'this' : used in base member initializer list

#ifndef ENABLE_EXTRA_WARNINGS
//#pragma warning (disable: 4101) // unreferenced local variable
//#pragma warning (disable: 4309) // truncation of constant value
#	pragma warning(disable : 4305)    // truncation from 'const int' to 'char'
#	pragma warning(disable : 4290)    // C++ exception specification ignored
									   // except to indicate a function is not
									   // __declspec(nothrow)
#endif

#include <windows.h>

#define USE_FMOPL_MIDI
#define USE_WINDOWS_MIDI