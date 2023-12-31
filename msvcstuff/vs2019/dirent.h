#pragma once

// povide a very basic implementation of the posix dirent for Windows
// implements just enough to be useful for Exult (checking if a dir exists)

typedef struct {
} DIR;

inline DIR* opendir(const char* path) {
	static DIR       ok;
	WIN32_FIND_DATAA data   = {0};
	auto             handle = FindFirstFileA(path, &data);
	if (handle != INVALID_HANDLE_VALUE
		&& (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		FindClose(handle);
		return &ok;
	}
	return 0;
}

inline void closedir(DIR*) {}