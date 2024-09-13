/*
 *  Copyright (C) 2001-2022  The Exult Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "version.h"

#include <cstdint>
#include <cstring>
#include <iostream>

// Only include gitinfo.h if it exists and none of the macros have already been
// defined
#if __has_include(                                                  \
		"gitinfo.h") && !defined(GIT_REVISION) && !defined(GIT_TAG) \
		&& !defined(GIT_REMOTE_BRANCH) && !defined(GIT_REMOTE_URL)
#	include "gitinfo.h"
#endif

#ifdef _WIN32
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	include <windows.h>

#	include <cstring>
#endif

#if (defined(__linux__) || defined(__linux) || defined(linux))
#	include <fstream>
#	include <string>
#endif

#ifdef _WIN32
// Do casts from pointers without too much undefined behavior.
template <typename To, typename From>
To safe_pointer_cast(From pointer) {
	// NOLINTNEXTLINE(bugprone-sizeof-expression)
	constexpr const size_t SizeFrom = sizeof(From);
	// NOLINTNEXTLINE(bugprone-sizeof-expression)
	constexpr const size_t SizeTo = sizeof(To);
	static_assert(
			std::is_pointer<From>::value && std::is_pointer<To>::value
					&& SizeFrom == SizeTo,
			"Pointer sizes do not match");
	To output;
	std::memcpy(
			static_cast<void*>(&output), static_cast<void*>(&pointer),
			SizeFrom);
	return output;
}

#endif
#ifdef GIT_REVISION
static const char git_rev[] = GIT_REVISION;
#else
static const char git_rev[] = "";
#endif
#ifdef GIT_TAG
static const char git_tag[] = GIT_TAG;
#else
static const char git_tag[] = "";
#endif
#ifdef GIT_REMOTE_BRANCH
static const char git_branch[] = GIT_REMOTE_BRANCH;
#else
static const char git_branch[] = "";
#endif
#ifdef GIT_REMOTE_URL
static const char git_url[] = GIT_REMOTE_URL;
#else
static const char git_url[] = "";
#endif

std::string_view VersionGetGitRevision(bool shortrev) {
	if (git_rev[0]) {
		if (shortrev) {
			return std::string_view(git_rev, 7);
		} else {
			return std::string_view(git_rev);
		}
	}
	return std::string_view();
}

std::string VersionGetGitInfo(bool limitedwidth) {
	// Everything for this function can be known at compile time
	// so could probably be made constexpr with a new enough c++ standard
	// and compiler support
	std::string result;
	result.reserve(256);

	if (git_branch[0]) {
		result += "Git Branch: ";
		result += git_branch;
		result += "\n";
	}
	if (git_rev[0]) {
		result += "Git Revision: ";
		result += VersionGetGitRevision(limitedwidth);
		result += "\n";
	}
	if (git_tag[0]) {
		result += "Git Tag: ";
		result += git_tag;
		result += "\n";
	}

	// Default to the Exult origin repo on github
	std::string src_url = "https://github.com/exult/exult";

	// if the remote url was supplied in the build make sure it is git hub https
	if (!std::strncmp(git_url, "https://github.com/", 19)) {
		src_url = git_url;
		// if it ends in '.git remove it
#ifdef __cpp_lib_starts_ends_with    // c++20
		if (src_url.ends_with(".git"))
#else    // older
		if (!src_url.compare(src_url.size() - 4, 4, ".git"))
#endif
		{
			src_url.resize(src_url.size() - 4);
		}
		// remove ending slash if any
		if (src_url.back() == '/') {
			src_url.pop_back();
		}

		// As we have github remote we can create url to exact revision/tag
		if (git_tag[0]) {
			src_url.reserve(src_url.size() + std::size(git_tag) + 7);
			if (limitedwidth) {
				src_url += "\n";
			}
			src_url += "/tree/";
			src_url += git_tag;
		} else if (git_rev[0]) {
			src_url.reserve(src_url.size() + std::size(git_rev) + 6);
			if (limitedwidth) {
				src_url += "\n";
			}
			src_url += "/tree/";
			src_url += VersionGetGitRevision(limitedwidth);
		}
	}

	result += "Source url: ";
	// if (limitedwidth) {
	//	result += "\n";
	// }
	result += src_url;
	return result;
}

void getVersionInfo(std::ostream& out) {
	/*
	 * 1. Exult version
	 */

	out << "Exult version " << VERSION << std::endl;
	/*
	 * 4. Git revision information
	 */
	out << VersionGetGitInfo(false) << std::endl;

	/*
	 * 3. Build Architechture
	 */
	out << "Build Architechture: ";

	// AMD64 x86_64
#if defined(__amd64__) || defined(__amd64) || defined(__amd64__) \
		|| defined(__amd64) || defined(_M_X64) || defined(_M_AMD64)
	out << "x86_64";
#	define WANT_CPUID
	// ARM THUMB
#elif defined(__thumb__) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARMT)
	out << "ARM Thumb";
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_ARM) \
		|| defined(_M_ARM) || defined(__arm)
	out << "ARM";
	// ARM64
#elif defined(__aarch64__) || defined(_M_ARM64)
	out << "ARM64";

	// X86
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i386) \
		|| defined(_M_IX86) || defined(__386)
	out << "x86";
#	define WANT_CPUID
#elif defined(__riscv)
	out << "RISC-V";
#else
	out << "unknown architechture update version.cc ";

#endif

	out << std::endl;

#ifdef WANT_CPUID
	OutputCPUID(&out);
#endif    //  WANT_CPUID

	/*
	 * 4. Build time
	 */

#if (defined(__TIME__) || defined(__DATE__))
	out << "Built at: ";
#	ifdef __DATE__
	out << __DATE__ << " ";
#	endif
#	ifdef __TIME__
	out << __TIME__;
#	endif
	out << std::endl;
#endif

	/*
	 * 5. Various important build options in effect
	 */

	out << "Compile-time options: ";
	bool firstoption = true;

#ifdef DEBUG
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "DEBUG";
#endif

#ifdef USE_TIMIDITY_MIDI
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "USE_TIMIDITY_MIDI";
#endif

#ifdef USE_FMOPL_MIDI
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "USE_FMOPL_MIDI";
#endif

#ifdef USE_MT32EMU_MIDI
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "USE_MT32EMU_MIDI";
#endif

#ifdef USE_ALSA_MIDI
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "USE_ALSA_MIDI";
#endif

#ifdef USE_EXULTSTUDIO
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "USE_EXULTSTUDIO";
#endif

#ifdef USECODE_DEBUGGER
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "USECODE_DEBUGGER";
#endif

#ifdef NO_SDL_PARACHUTE
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "NO_SDL_PARACHUTE";
#endif

#ifdef HAVE_ZIP_SUPPORT
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "HAVE_ZIP_SUPPORT";
#endif

#ifdef ENABLE_MIDISFX
	if (!firstoption) {
		out << ", ";
	}
	firstoption = false;
	out << "ENABLE_MIDISFX";
#endif

	if (firstoption) {
		out << "(none)";
	}
	out << std::endl;

	/*
	 * 6. Compiler used to create this binary
	 */

	out << "Compiler: ";
#ifdef __INTEL_COMPILER
#	define COMPILER __VERSION__
#elif defined(__clang__)
#	define COMPILER __VERSION__
#elif defined(__GNUC__)
#	define COMPILER "GCC " __VERSION__
#elif defined(_MSC_FULL_VER)
#	define COMPILER                                                       \
		"Microsoft C/C++ Compiler " << (_MSC_FULL_VER / 10'000'000) << '.' \
									<< ((_MSC_FULL_VER / 100'000) % 100)   \
									<< '.' << ((_MSC_FULL_VER % 100'000))
#else
#	define COMPILER "unknown compiler"
#endif

	out << COMPILER << std::endl;
#undef COMPILER

	/*
	 * 7. Platform
	 */

	out << std::endl << "Platform: ";

#if (defined(ANDROID))
	out << "Android";
#elif (defined(__linux__) || defined(__linux) || defined(linux))
	std::string ver;

	try {
		std::ifstream procversion("/proc/version");
		if (!procversion) {
			ver = "Linux";
		} else {
			std::getline(procversion, ver);
			procversion.close();
			ver = ver.substr(0, ver.find('('));
		}
	} catch (...) {
		ver = "Linux";
	}
	out << ver;
#elif (defined(__sun__) || defined(__sun))
	out << "Solaris";
#elif (defined(_WIN32))
	out << "Windows ";
	{
		// Get the version
		OSVERSIONINFOEXA info;
		ZeroMemory(&info, sizeof(info));
		info.dwOSVersionInfoSize = sizeof(info);
		GetVersionExA(safe_pointer_cast<LPOSVERSIONINFOA>(&info));

		// Platform is NT
		if (info.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			if (info.wProductType == VER_NT_WORKSTATION) {
				if (info.dwMajorVersion < 4) {
					out << "NT";
				} else if (info.dwMajorVersion == 4) {
					out << "NT4";
				} else if (
						info.dwMajorVersion == 5 && info.dwMinorVersion == 0) {
					out << 2000;
				} else if (
						info.dwMajorVersion == 5 && info.dwMinorVersion == 1) {
					out << "XP";
				} else if (
						info.dwMajorVersion == 5 && info.dwMinorVersion == 2) {
					// Only workstation release with version 5.2 was XP x64
					out << "XP";
				} else if (
						info.dwMajorVersion == 6 && info.dwMinorVersion == 0) {
					out << "Vista";
				} else if (
						info.dwMajorVersion == 6 && info.dwMinorVersion == 1) {
					out << "7";
				} else if (
						info.dwMajorVersion == 6 && info.dwMinorVersion == 2) {
					out << "8";
				} else {
					// Note: Without the proper manifest file, GetVersionEx will
					// lie about these.
					if (info.dwMajorVersion == 6 && info.dwMinorVersion == 3) {
						out << "8.1";
					} else if (info.dwMajorVersion == 10) {
						// cut off for Windows 10 and 11 is build 22000 (11
						// builds with a build number lower than 22000 are not
						// public releases so I don't care)
						if (LOWORD(info.dwBuildNumber & 0xFFFF) < 22000) {
							out << "10";
						} else {
							out << "11";
						}
					} else {
						out << "Unknown NT";
					}
				}
			} else {
				// Server
				// Note: No Server release of 5.1
				if (info.dwMajorVersion < 4) {
					out << "NT Server";
				} else if (info.dwMajorVersion == 4) {
					out << "NT4 Server";
				} else if (
						info.dwMajorVersion == 5 && info.dwMinorVersion == 0) {
					out << "2000 Server";
				} else if (
						info.dwMajorVersion == 5 && info.dwMinorVersion == 2) {
					out << " Windows Server 2003";
				} else if (
						info.dwMajorVersion == 6 && info.dwMinorVersion == 0) {
					out << "Windows Server 2008";
				} else if (
						info.dwMajorVersion == 6 && info.dwMinorVersion == 1) {
					out << "Windows Server 2008 R2";
				} else if (
						info.dwMajorVersion == 6 && info.dwMinorVersion == 2) {
					out << "Windows Server 2012";
				} else {
					// Note: Without the proper manifest file, GetVersionEx will
					// lie about these.
					if (info.dwMajorVersion == 6 && info.dwMinorVersion == 3) {
						out << "Windows Server 2012 R2";
					} else if (info.dwMajorVersion == 10) {
						if (LOWORD(info.dwBuildNumber & 0xFFFF) < 17000) {
							out << "Server 2016";
						} else if (
								LOWORD(info.dwBuildNumber & 0xFFFF) < 19000) {
							out << "Server 2019";
						} else {
							out << "Server 2022";
						}
					} else {
						out << "Unknown NT Server";
					}
				}
			}
			if (info.szCSDVersion[0] != 0) {
				out << " " << info.szCSDVersion;
			}

		} else {
			out << "Windows ";

			if (info.dwMajorVersion == 4 && info.dwMinorVersion == 0) {
				out << 95;
				if (info.szCSDVersion[1] != ' ') {
					out << info.szCSDVersion;
				}
			} else if (info.dwMajorVersion == 4 && info.dwMinorVersion == 10) {
				out << 98;
				if (info.szCSDVersion[1] == 'A') {
					out << " SE";
				} else if (info.szCSDVersion[1] != ' ') {
					out << info.szCSDVersion;
				}
			} else if (info.dwMajorVersion == 4 && info.dwMinorVersion == 90) {
				out << "Me";
			}
		}
		out << " Version " << info.dwMajorVersion << "." << info.dwMinorVersion
			<< " Build " << LOWORD(info.dwBuildNumber & 0xFFFF) << " ";

		// This function only exists in XP or newer but I see no reason to break
		// compatibility with older windows version here so using it dynamically
		void(WINAPI * fpGetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo)
				= nullptr;
		HMODULE kernel32 = GetModuleHandleA("KERNEL32");
		if (kernel32 != nullptr) {
			using LPNativeSystemInfo = decltype(fpGetNativeSystemInfo);
			fpGetNativeSystemInfo    = safe_pointer_cast<LPNativeSystemInfo>(
                    GetProcAddress(kernel32, "GetNativeSystemInfo"));
			// We default to GetSystemInfo (win2000 req) if we couldn't get
			// GetNativeSystemInfo
			if (fpGetNativeSystemInfo == nullptr) {
				fpGetNativeSystemInfo = safe_pointer_cast<LPNativeSystemInfo>(
						GetProcAddress(kernel32, "GetSystemInfo"));
			}
		}

		// If we have one of the get systeminfo functions print some details,
		// mostly to know if its ARM64 or AMD64 OS running x86 code
		if (fpGetNativeSystemInfo != nullptr) {
			SYSTEM_INFO sysinfo;
			fpGetNativeSystemInfo(&sysinfo);
			switch (sysinfo.wProcessorArchitecture) {
			case PROCESSOR_ARCHITECTURE_INTEL:
				out << "x86 ";
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				out << "IA64 ";
				break;
#	if defined(PROCESSOR_ARCHITECTURE_ARM64)    // This only exists in Windows
			case PROCESSOR_ARCHITECTURE_ARM64:    // 10 sdks or newer, Will fail
#	else                                        // with vista, 7 or 8 sdks
			case 12:
#	endif
				out << "ARM64 ";
				break;

			case PROCESSOR_ARCHITECTURE_ARM:
				out << "ARM ";
				break;
			case PROCESSOR_ARCHITECTURE_AMD64:
				out << "x64 ";
				break;
			default:
				out << "UnknownArch ";
			}

			out << sysinfo.dwNumberOfProcessors << " Processors";
		}
	}
#elif (defined(FREEBSD))
	out << "FreeBSD";
#elif (defined(MACOSX))
	out << "macOS";
#elif (defined(__IPHONEOS__))
	out << "iOS";
#elif (defined(NETBSD))
	out << "NetBSD";
#else
	out << "Unknown";
#endif

	out << std::endl;
}

const int CPUID_EAX = 0;
const int CPUID_EBX = 1;
const int CPUID_ECX = 2;
const int CPUID_EDX = 3;
using std::uint32_t;
#ifdef WANT_CPUID
// CPUID support for x86 cpus, wont work for 386 or 486 cpus but I don't think
// we'd support those anyway
#	if __has_include(<cpuid.h>)
// gcc and cland should have this
#		include <cpuid.h>

bool CPUID(uint32_t leaf, uint32_t subleaf, uint32_t regs[4]) {
	return __get_cpuid_count(
				   leaf, subleaf, &regs[CPUID_EAX], &regs[CPUID_EBX],
				   &regs[CPUID_ECX], &regs[CPUID_EDX])
		   != 0;
}
#	elif __has_include(<intrin.h>)
// MSVC should have this
#		include <intrin.h>

bool CPUID(uint32_t leaf, uint32_t subleaf, uint32_t regs[4]) {
	__cpuidex(safe_pointer_cast<int*>(regs), leaf, subleaf);
	return true;
}
#	else
#		define NO_CPUID
#	endif

#else
#	define NO_CPUID
#endif

#ifdef NO_CPUID
bool CPUID(uint32_t leaf, uint32_t subleaf, uint32_t regs[4]) {
	return false;
}
#endif

uint32_t CPUID_0(char vendor[13]) {
	uint32_t regs[4] = {0};
	if (!CPUID(0, 0, regs)) {
		vendor[0] = 0;
		return 0;
	}

	memcpy(&vendor[0], &regs[CPUID_EBX], 4);
	memcpy(&vendor[4], &regs[CPUID_EDX], 4);
	memcpy(&vendor[8], &regs[CPUID_ECX], 4);
	vendor[12] = 0;
	return regs[CPUID_EAX];
}

uint32_t CPUID_ExBrand(char brand[49]) {
	brand[0]         = 0;
	uint32_t regs[4] = {0};
	if (!CPUID(0x80000000, 0, regs)) {
		return 0;
	}
	uint32_t max_ex = regs[CPUID_EAX];

	for (int i = 0; (i + 0x80000002) <= max_ex && i < 3; i++) {
		if (!CPUID(0x80000002 + i, 0, regs)) {
			break;
		}

		memcpy(&brand[i * 16 + 0], &regs[CPUID_EAX], 4);
		memcpy(&brand[i * 16 + 4], &regs[CPUID_EBX], 4);
		memcpy(&brand[i * 16 + 8], &regs[CPUID_ECX], 4);
		memcpy(&brand[i * 16 + 12], &regs[CPUID_EDX], 4);
		brand[i * 16 + 16] = 0;
	}
	return max_ex;
}

static bool cpuidunsupportedfeatures = false;

bool OutputCPUID(std::ostream* out) {
	cpuidunsupportedfeatures = false;
	using std::endl;
	char     vendor[13];
	uint32_t maxcpuid = CPUID_0(vendor);
	if (maxcpuid == 0) {
		return false;
	}
	char     brand[49];
	uint32_t max_ex = CPUID_ExBrand(brand);
	if (out) {
		*out << "CPUID: " << vendor << ", " << brand << endl;
	}

	uint32_t regs[4] = {0};
	if (!CPUID(1, 0, regs)) {
		std::memset(regs, 0, sizeof(regs));
	}

	// Just outputting raw fields here instead of getting real values
	if (out) {
		*out << "stepping: " << ((regs[CPUID_EAX] >> 0) & 0xf) << endl;
	}
	if (out) {
		*out << "type: " << ((regs[CPUID_EAX] >> 12) & 0x3) << endl;
	}
	if (out) {
		*out << "family: " << ((regs[CPUID_EAX] >> 8) & 0xf) << endl;
	}
	if (out) {
		*out << "ext family: " << ((regs[CPUID_EAX] >> 20) & 0xff) << endl;
	}
	if (out) {
		*out << "model: " << ((regs[CPUID_EAX] >> 4) & 0xf) << endl;
	}
	if (out) {
		*out << "ext model: " << ((regs[CPUID_EAX] >> 16) & 0xf) << endl;
	}

	uint32_t regs_ex[4] = {0};
	if (max_ex < 0x80000001 || !CPUID(0x80000001, 0, regs_ex)) {
		std::memset(regs_ex, 0, sizeof(regs_ex));
	}

	uint32_t regs_efb[4] = {0};
	if (maxcpuid < 7 || !CPUID(7, 0, regs_efb)) {
		std::memset(regs_efb, 0, sizeof(regs_efb));
	}

	bool need_sse    = false;
	bool need_sse2   = false;
	bool need_sse3   = false;    // x86-64-v2
	bool need_ssse3  = false;    // x86-64-v2
	bool need_sse4_1 = false;    // x86-64-v2
	bool need_sse4_2 = false;    // x86-64-v2
	bool need_sse4a  = false;
#ifdef __SSE__
	need_sse = true;
#endif
#ifdef __SSE_MATH__
	need_sse = true;
#endif
#ifdef __SSE2__
	need_sse2 = true;
#endif
#ifdef __SSE2_MATH__
	need_sse2 = true;
#endif
#ifdef __SSE3__
	need_sse3 = true;
#endif
#ifdef __SSSE3__
	need_ssse3 = true;
#endif
#ifdef __SSE4_1__
	need_sse4_1 = true;
#endif
#ifdef __SSE4_2__
	need_sse4_2 = true;
#endif

#ifdef __SSE4A__
	need_sse4a = true;
#endif

	bool need_avx      = false;    // x86-64-v3
	bool need_avx2     = false;    // x86-64-v3
	bool need_avx512f  = false;    // x86-64-v4
	bool need_avx512bw = false;    // x86-64-v4
	bool need_avx512vl = false;    // x86-64-v4
	bool need_avx512cd = false;    // x86-64-v4
	bool need_avx512dq = false;    // x86-64-v4
#ifdef __AVX__
	need_avx = true;
#endif
#ifdef __AVX2__
	need_avx2 = true;
#endif
#ifdef __AVX512F__
	need_avx512f = true;
#endif
#ifdef __AVX512BW__
	need_avx512bw = true;
#endif
#ifdef __AVX512VL__
	need_avx512vl = true;
#endif
#ifdef __AVX512CD__
	need_avx512cd = true;
#endif
#ifdef __AVX512DQ__
	need_avx512dq = true;
#endif

	bool need_movbe = false;    // likely means x86-64-v3 build
#ifdef __MOVBE__
	need_movbe = true;
#endif
	bool need_popcnt = false;    // likely means x86-64-v2 build
#ifdef __POPCNT__
	need_popcnt = true;
#endif
	bool need_fma = false;    // likely means x86-64-v3 build
#ifdef __FMA__
	need_fma = true;
#endif
	bool need_lahfsahf = false;    // likely means x86-64-v2 build
#ifdef __LAHF_SAHF__
	need_lahfsahf = sizeof(void*) == 8;    // 32 bit x86 always supports this so
										   // only check for it in 64 bit builds
#endif
	bool need_bmi1 = false;    // likely means x86-64-v3 build
#ifdef __BMI1__
	need_bmi1 = true;
#endif
	bool need_bmi2 = false;    // likely means x86-64-v3 build
#ifdef __BMI2__
	need_bmi2 = true;
#endif
	bool need_lzcnt = false;
#ifdef __LZCNT__
	need_lzcnt = true;
#endif

	// MSVC x86 floating point mode
#ifdef _M_IX86_FP
#	if _M_IX86_FP == 1
	need_sse = true;
#	elif _M_IX86_FP == 2
	need_sse  = true;
	need_sse2 = true;
#	endif
#endif

	// Check for required features
	if (need_sse) {
		if (regs[CPUID_EDX] & (1 << 25)) {
			if (out) {
				*out << "SSE: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSE REQUIRED BUT NOT SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_sse2) {
		if (regs[CPUID_EDX] & (1 << 26)) {
			if (out) {
				*out << "SSE2: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSE2 REQUIRED BUT NOT SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_sse3) {
		if (regs[CPUID_ECX] & 1) {
			if (out) {
				*out << "SSE3: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSE3 REQUIRED BUT NOT SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_ssse3) {
		if (regs[CPUID_ECX] & (1 << 9)) {
			if (out) {
				*out << "SSSE3: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSSE3 REQUIRED BUT NOT SUPPORTED BY "
						"THIS CPU"
					 << endl;
			}
		}
	}
	if (need_sse4_1) {
		if (regs[CPUID_ECX] & (1 << 19)) {
			if (out) {
				*out << "SSE4.1: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSE4.1 REQUIRED BUT NOT SUPPORTED "
						"BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_sse4_2) {
		if (regs[CPUID_ECX] & (1 < 20)) {
			if (out) {
				*out << "SSE4.2: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSE4.2 REQUIRED BUT NOT "
						"SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_sse4a) {
		if (regs[CPUID_ECX] & (1 < 6)) {
			if (out) {
				*out << "SSE4A: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING SSE4A REQUIRED BUT NOT "
						"SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_movbe) {
		if (regs[CPUID_ECX] & (1 << 22)) {
			if (out) {
				*out << "MOVBE: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING MOVBE REQUIRED BUT "
						"NOT SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_popcnt) {
		if (regs[CPUID_ECX] & (1 << 23)) {
			if (out) {
				*out << "POPCNT: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING POPCNT REQUIRED "
						"BUT NOT SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_fma) {
		if (regs[CPUID_ECX] & (1 << 12)) {
			if (out) {
				*out << "FMA: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING FMA REQUIRED "
						"BUT NOT SUPPORTED BY "
						"THIS CPU"
					 << endl;
			}
		}
	}
	if (need_lahfsahf) {
		if (regs_ex[CPUID_ECX] & 1) {
			if (out) {
				*out << "LAHF_SAHF: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING LAHF_SAHF "
						"REQUIRED BUT NOT "
						"SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_lzcnt) {
		if (regs_ex[CPUID_ECX] & (1 << 5)) {
			if (out) {
				*out << "LZCNT: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING LZCNT "
						"REQUIRED BUT NOT "
						"SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_bmi1) {
		if (regs_efb[CPUID_EBX] & (1 << 3)) {
			if (out) {
				*out << "BMI1: "
						"supported"
					 << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING BMI1 "
						"REQUIRED BUT "
						"NOT SUPPORTED "
						"BY THIS CPU"
					 << endl;
			}
		}
	}
	if (need_bmi2) {
		if (regs_efb[CPUID_EBX] & (1 << 8)) {
			if (out) {
				*out << "BMI2: "
						"supported"
					 << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING "
						"BMI2 "
						"REQUIRED "
						"BUT NOT "
						"SUPPORTED "
						"BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_avx) {
		if (regs[CPUID_ECX] & (1 << 28)) {
			if (out) {
				*out << "AVX: "
						"suppor"
						"ted"
					 << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNIN"
						"G AVX "
						"REQUIR"
						"ED "
						"BUT "
						"NOT "
						"SUPPOR"
						"TED "
						"BY "
						"THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_avx2) {
		if (regs_efb[CPUID_EBX] & (1 << 5)) {
			if (out) {
				*out << "AVX2: "
						"suppor"
						"ted"
					 << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNIN"
						"G "
						"AVX2 "
						"REQUIR"
						"ED "
						"BUT "
						"NOT "
						"SUPPOR"
						"TED "
						"BY "
						"THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_avx512f) {
		if (regs_efb[CPUID_EBX] & (1 << 16)) {
			if (out) {
				*out << "AV"
						"X5"
						"12"
						"F:"
						" s"
						"up"
						"po"
						"rt"
						"ed"
					 << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WA"
						"RN"
						"IN"
						"G "
						"AV"
						"X5"
						"12"
						"F "
						"RE"
						"QU"
						"IR"
						"ED"
						" B"
						"UT"
						" N"
						"OT"
						" S"
						"UP"
						"PO"
						"RT"
						"ED"
						" B"
						"Y "
						"TH"
						"IS"
						" C"
						"PU"
					 << endl;
			}
		}
	}
	if (need_avx512dq) {
		if (regs_efb[CPUID_EBX] & (1 << 17)) {
			if (out) {
				*out << "AVX512DQ: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING AVX512DQ REQUIRED BUT NOT SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_avx512bw) {
		if (regs_efb[CPUID_EBX] & (1 << 30)) {
			if (out) {
				*out << "AVX512BW: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING AVX512BW REQUIRED BUT NOT SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_avx512vl) {
		if (regs_efb[CPUID_EBX] & (1 << 31)) {
			if (out) {
				*out << "AVX512VL: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING AVX512VL REQUIRED BUT NOT SUPPORTED BY THIS "
						"CPU"
					 << endl;
			}
		}
	}
	if (need_avx512cd) {
		if (regs_efb[CPUID_EBX] & (1 << 28)) {
			if (out) {
				*out << "AVX512CD: supported" << endl;
			}
		} else {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING AVX512CD REQUIRED BUT NOT SUPPORTED BY "
						"THIS "
						"CPU"
					 << endl;
			}
		}
	}

	// x86-64
	// feature
	// levels
	if (sizeof(void*) == 8) {
		bool v2      = true;
		bool need_v2 = need_popcnt && need_sse3 && need_ssse3 && need_sse4_1
					   && need_sse4_2 && need_lahfsahf;
		// CMPXCHG16B
		if (!(regs[CPUID_ECX] & (1 << 28))) {
			v2 = false;
		}
		// LAHF-SAHF
		else if (!(regs_ex[CPUID_ECX] & 1)) {
			v2 = false;
		}    // POPCNT
		else if (!(regs[CPUID_ECX] & (1 << 23))) {
			v2 = false;
		}
		// SSE3
		else if (!(regs[CPUID_ECX] & 1)) {
			v2 = false;
		}
		// SSE4_1
		else if (!(regs[CPUID_ECX] & (1 << 19))) {
			v2 = false;
		}
		// SSE4_2
		else if (!(regs[CPUID_ECX] & (1 << 22))) {
			v2 = false;
		}
		// SSSE3
		else if (!(regs[CPUID_ECX] & (1 << 9))) {
			v2 = false;
		}

		bool v3      = v2;
		bool need_v3 = need_avx && need_avx2 && need_movbe && need_fma
					   && need_bmi2 && need_bmi1 && need_lzcnt;
		// AVX
		if (!(regs[CPUID_ECX] & (1 << 28))) {
			v3 = false;
		}
		// AVX2
		else if (!(regs_efb[CPUID_EBX] & (1 << 5))) {
			v3 = false;
		}
		// BMI1
		else if (!(regs_efb[CPUID_EBX] & (1 << 3))) {
			v3 = false;
		}
		// BMI2
		else if (!(regs_efb[CPUID_EBX] & (1 << 8))) {
			v3 = false;
		}
		// F16C
		else if (!(regs[CPUID_ECX] & (1 << 29))) {
			v3 = false;
		}
		// FMA
		else if (!(regs[CPUID_ECX] & (1 << 12))) {
			v3 = false;
		}
		// LZCNT
		else if (!(regs_ex[CPUID_ECX] & (1 << 5))) {
			v3 = false;
		}
		// MOVBE
		else if (!(regs[CPUID_ECX] & (1 << 22))) {
			v3 = false;
		}
		// OSXSAVE
		else if (!(regs[CPUID_ECX] & (1 << 27))) {
			v3 = false;
		}

		bool v4      = v3;
		bool need_v4 = need_avx512cd && need_avx512vl && need_avx512f
					   && need_avx512dq && need_avx512bw;
		// avx512-cd
		if (!(regs_efb[CPUID_EBX] & (1 << 28))) {
			v4 = false;
			// avx512-vl
		} else if (!(regs_efb[CPUID_EBX] & (1 << 31))) {
			v4 = false;
			// avx512-bw
		} else if (!(regs_efb[CPUID_EBX] & (1 << 30))) {
			v4 = false;
			// avx512-f
		} else if (!(regs_efb[CPUID_EBX] & (1 << 16))) {
			v4 = false;
			// avx512-dq
		} else if (!(regs_efb[CPUID_EBX] & (1 << 17))) {
			v4 = false;
		}

		if (v4) {
			if (out) {
				*out << "X86-64-v4 supported" << endl;
			}
		} else if (v3) {
			if (out) {
				*out << "X86-64-v3 supported" << endl;
			}
		} else if (v2) {
			if (out) {
				*out << "X86-64-v2 supported" << endl;
			}
		} else {
			if (out) {
				*out << "Baseline X86-64 supported" << endl;
			}
		}

		if (need_v4 && !v4) {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING X86-64-v4 FEATURES ARE REQUIRED BY "
						"THIS BUILD "
						"BUT "
						"ARE NOT SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
		if (need_v3 && !v3) {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING X86-64-v3 FEATURES ARE REQUIRED "
						"BY THIS BUILD "
						"BUT "
						"ARE NOT SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
		if (need_v2 && !v2) {
			cpuidunsupportedfeatures = true;
			if (out) {
				*out << "WARNING X86-64-v2 FEATURES ARE "
						"REQUIRED BY THIS BUILD "
						"BUT "
						"ARE NOT SUPPORTED BY THIS CPU"
					 << endl;
			}
		}
	}

	return true;
}

bool CPUIDHasUnsuportedFeatures() {
	OutputCPUID(nullptr);
	return cpuidunsupportedfeatures;
}
