/*
 *  Copyright (C) 2020 Marzo Sette Torres Junior
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

#ifndef ENDIANIO_H
#define ENDIANIO_H

#include <cstdint>
#include <cstring>
#include <ios>
#include <type_traits>

// Need to define manually for MSVC.
#if defined(_MSC_VER) || defined(_WIN32)
#	ifndef __ORDER_BIG_ENDIAN__
#		define __ORDER_BIG_ENDIAN__ 4321
#	endif
#	ifndef __ORDER_LITTLE_ENDIAN__
#		define __ORDER_LITTLE_ENDIAN__ 1234
#	endif
#	ifndef __BYTE_ORDER__
#		define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#	endif
#endif

#if !defined(__BYTE_ORDER__)                          \
		|| (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__ \
			&& __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__)
#	error "Byte order is neither little endian nor big endian. Do not know how to proceed."
#endif

#if defined(_MSC_VER)
#	define ENDIANIO_INLINE       __forceinline
#	define ENDIANIO_CONST_INLINE __forceinline
#	define ENDIANIO_PURE_INLINE  __forceinline
#elif defined(__GNUG__)
#	define ENDIANIO_INLINE       [[gnu::always_inline]] inline
#	define ENDIANIO_CONST_INLINE [[using gnu: const, always_inline]] inline
#	define ENDIANIO_PURE_INLINE  [[using gnu: pure, always_inline]] inline
#else
#	define ENDIANIO_INLINE       inline
#	define ENDIANIO_CONST_INLINE inline
#	define ENDIANIO_PURE_INLINE  inline
#endif

// Some things cannot be constexpr on MSVC compiler.
#ifdef __GNUG__
#	define ENDIANIO_CONSTEXPR constexpr
#else
#	define ENDIANIO_CONSTEXPR
#endif

#if defined(_MSC_VER)
#	include <intrin.h>
#	include <stdlib.h>
#	pragma intrinsic(_byteswap_ushort)
#	pragma intrinsic(_byteswap_ulong)
#	pragma intrinsic(_byteswap_uint64)
#endif

namespace {    // anonymous

	namespace detail {
		// is_detected and related (ported from Library Fundamentals v2).
		namespace inner {
			template <
					typename Default, typename AlwaysVoid,
					template <typename...> class Op, typename... Args>
			struct detector {
				using value_t = std::false_type;
				using type    = Default;
			};

			template <
					typename Default, template <typename...> class Op,
					typename... Args>
			struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
				using value_t = std::true_type;
				using type    = Op<Args...>;
			};

			struct nonesuch final {
				nonesuch(const nonesuch&)                    = delete;
				nonesuch(nonesuch&&)                         = delete;
				nonesuch()                                   = delete;
				~nonesuch()                                  = delete;
				auto operator=(const nonesuch&) -> nonesuch& = delete;
				auto operator=(nonesuch&&) -> nonesuch&      = delete;
			};
		}    // namespace inner

		template <template <typename...> class Op, typename... Args>
		using is_detected = typename inner::detector<
				inner::nonesuch, void, Op, Args...>::value_t;

		template <template <typename...> class Op, typename... Args>
		constexpr bool is_detected_v = is_detected<Op, Args...>::value;

		template <template <typename...> class Op, typename... Args>
		constexpr bool exists = is_detected_v<Op, Args...>;

		namespace functions {
			template <typename T>
			using read = decltype(std::declval<T>().read(
					std::declval<char*>(), std::declval<std::streamsize>()));

			template <typename T>
			using write = decltype(std::declval<T>().write(
					std::declval<const char*>(),
					std::declval<std::streamsize>()));
		}    // namespace functions

		// Robust SFINAE idiom by Jonathan Boccara
		// https://www.fluentcpp.com/2019/08/23/how-to-make-sfinae-pretty-and-robust/
		template <typename T>
		using has_read_function_t
				= std::enable_if_t<exists<functions::read, T>, bool>;

		template <typename T>
		using has_write_function_t
				= std::enable_if_t<exists<functions::write, T>, bool>;

		template <typename T>
		using is_an_integer_t = std::enable_if_t<
				std::is_integral_v<T> && !std::is_same_v<T, bool>, bool>;

		template <typename T>
		struct is_byte_type_impl : std::integral_constant<bool, false> {};

		template <>
		struct is_byte_type_impl<char> : std::integral_constant<bool, true> {};

		template <>
		struct is_byte_type_impl<unsigned char>
				: std::integral_constant<bool, true> {};

		template <>
		struct is_byte_type_impl<std::byte>
				: std::integral_constant<bool, true> {};

		template <typename T>
		using is_byte_type_t = std::enable_if_t<
				is_byte_type_impl<std::remove_cv_t<T>>::value, bool>;

		enum class endian {
			big    = __ORDER_BIG_ENDIAN__,
			little = __ORDER_LITTLE_ENDIAN__,
			native = __BYTE_ORDER__,
		};

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint8_t
				bswap(uint8_t val) noexcept {
			return val;
		}

#if defined(__GNUG__)
#	ifdef __INTEL_COMPILER
		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint16_t
				bswap(uint16_t val) noexcept {
			if (__builtin_constant_p(val) != 0) {
				return __bswap_constant_16(val);
			}
			return __builtin_bswap16(val);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint32_t
				bswap(uint32_t val) noexcept {
			if (__builtin_constant_p(val) != 0) {
				return __bswap_constant_32(val);
			}
			return __builtin_bswap32(val);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint64_t
				bswap(uint64_t val) noexcept {
			if (__builtin_constant_p(val) != 0) {
				return __bswap_constant_64(val);
			}
			return __builtin_bswap64(val);
		}
#	else

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint16_t
				bswap(uint16_t val) noexcept {
			return __builtin_bswap16(val);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint32_t
				bswap(uint32_t val) noexcept {
			return __builtin_bswap32(val);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint64_t
				bswap(uint64_t val) noexcept {
			return __builtin_bswap64(val);
		}

#	endif
#elif defined(_MSC_VER)
		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint16_t
				bswap(uint16_t val) noexcept {
			return _byteswap_ushort(val);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint32_t
				bswap(uint32_t val) noexcept {
			return _byteswap_ulong(val);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint64_t
				bswap(uint64_t val) noexcept {
			return _byteswap_uint64(val);
		}
#else
		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint16_t
				bswap(uint16_t val) noexcept {
			return ((val & 0xffu) << 8) | ((val >> 8) & 0xffu);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint32_t
				bswap(uint32_t val) noexcept {
			val = ((val & 0xffffu) << 16) | ((val >> 16) & 0xffffu);
			return ((val & 0xff00ffu) << 8) | ((val >> 8) & 0xff00ffu);
		}

		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR uint64_t
				bswap(uint64_t val) noexcept {
			val = ((val & 0xffffffffull) << 32) | ((val >> 32) & 0xffffffffull);
			val = ((val & 0xffff0000ffffull) << 16)
				  | ((val >> 16) & 0xffff0000ffffull);
			return ((val & 0xff00ff00ff00ffull) << 8)
				   | ((val >> 8) & 0xff00ff00ff00ffull);
		}
#endif

		template <
				typename Int,
				std::enable_if_t<std::is_signed_v<Int>, bool> = true>
		ENDIANIO_CONST_INLINE ENDIANIO_CONSTEXPR Int bswap(Int val) noexcept {
			using UT = std::make_unsigned_t<Int>;
			return static_cast<Int>(bswap(static_cast<UT>(val)));
		}

#ifdef __clang__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wunused-template"
#endif
		struct EndianBaseImpl {
			template <
					endian FromEndian, typename Int, typename Byte,
					is_an_integer_t<Int> = true, is_byte_type_t<Byte> = true>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static Int Read(Byte*&& source) {
				Int val;
				std::memcpy(&val, source, sizeof(Int));
				if constexpr (FromEndian != endian::native) {
					val = bswap(val);
				}
				return val;
			}

			template <
					endian FromEndian, typename Int, typename Byte,
					is_an_integer_t<Int> = true, is_byte_type_t<Byte> = true>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static Int Read(Byte*& source) {
				Int val;
				std::memcpy(&val, source, sizeof(Int));
				std::advance(source, sizeof(Int));
				if constexpr (FromEndian != endian::native) {
					val = bswap(val);
				}
				return val;
			}

			template <
					endian FromEndian, typename Int, typename Stream,
					has_read_function_t<Stream> = true>
			ENDIANIO_INLINE static auto Read(Stream& in) {
				alignas(alignof(Int))
						typename Stream::char_type buffer[sizeof(Int)];
				in.read(std::begin(buffer), sizeof(Int));
				return Read<FromEndian, Int>(std::begin(buffer));
			}

			template <
					endian FromEndian, typename Int, typename Stream,
					has_read_function_t<Stream> = true>
			ENDIANIO_INLINE static auto Read(Stream* in) {
				alignas(alignof(Int))
						typename Stream::char_type buffer[sizeof(Int)];
				in->read(std::begin(buffer), sizeof(Int));
				return Read<FromEndian, Int>(std::begin(buffer));
			}

			template <
					endian ToEndian, typename Int, typename Byte,
					is_an_integer_t<Int> = true, is_byte_type_t<Byte> = true>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write(
					Byte*&& dest, Int val) {
				if constexpr (ToEndian != endian::native) {
					val = bswap(val);
				}
				std::memcpy(dest, &val, sizeof(Int));
			}

			template <
					endian ToEndian, typename Int, typename Byte,
					is_an_integer_t<Int> = true, is_byte_type_t<Byte> = true>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write(
					Byte*& dest, Int val) {
				if constexpr (ToEndian != endian::native) {
					val = bswap(val);
				}
				std::memcpy(dest, &val, sizeof(Int));
				std::advance(dest, sizeof(Int));
			}

			template <
					endian ToEndian, typename Int, typename Stream,
					has_write_function_t<Stream> = true>
			ENDIANIO_INLINE static auto Write(Stream& out, Int val) {
				alignas(alignof(Int))
						typename Stream::char_type buffer[sizeof(Int)];
				Write<ToEndian>(std::begin(buffer), val);
				out.write(std::cbegin(buffer), sizeof(Int));
			}

			template <
					endian ToEndian, typename Int, typename Stream,
					has_write_function_t<Stream> = true>
			ENDIANIO_INLINE static auto Write(Stream* out, Int val) {
				alignas(alignof(Int))
						typename Stream::char_type buffer[sizeof(Int)];
				Write<ToEndian>(std::begin(buffer), val);
				out->write(std::cbegin(buffer), sizeof(Int));
			}
		};

#ifdef __clang__
#	pragma GCC diagnostic pop
#endif

		template <endian type>
		struct endian_base {
			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read2(Src&& in) {
				return EndianBaseImpl::Read<type, uint16_t>(
						std::forward<Src>(in));
			}

			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read2s(Src&& in) {
				return EndianBaseImpl::Read<type, int16_t>(
						std::forward<Src>(in));
			}

			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read4(Src&& in) {
				return EndianBaseImpl::Read<type, uint32_t>(
						std::forward<Src>(in));
			}

			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read4s(Src&& in) {
				return EndianBaseImpl::Read<type, int32_t>(
						std::forward<Src>(in));
			}

			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read8(Src&& in) {
				return EndianBaseImpl::Read<type, uint64_t>(
						std::forward<Src>(in));
			}

			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read8s(Src&& in) {
				return EndianBaseImpl::Read<type, int64_t>(
						std::forward<Src>(in));
			}

			template <typename Src>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto ReadPtr(Src&& in) {
				return EndianBaseImpl::Read<type, uintptr_t>(
						std::forward<Src>(in));
			}

			template <typename Int, typename Src, is_an_integer_t<Int> = true>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto ReadN(Src&& in) {
				return EndianBaseImpl::Read<type, Int>(std::forward<Src>(in));
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write2(
					Dst&& out, uint16_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write2s(
					Dst&& out, int16_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write4(
					Dst&& out, uint32_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write4s(
					Dst&& out, int32_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write8(
					Dst&& out, uint64_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write8s(
					Dst&& out, int64_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Dst>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void WritePtr(
					Dst&& out, uintptr_t val) {
				EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}

			template <typename Int, typename Dst, is_an_integer_t<Int> = true>
			ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto WriteN(
					Dst&& out, Int val) {
				return EndianBaseImpl::Write<type>(std::forward<Dst>(out), val);
			}
		};
}}    // namespace ::detail

using big_endian    = detail::endian_base<detail::endian::big>;
using little_endian = detail::endian_base<detail::endian::little>;
using native_endian = detail::endian_base<detail::endian::native>;

template <typename Src>
ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read1(Src&& in) {
	return detail::EndianBaseImpl::Read<detail::endian::native, uint8_t>(
			std::forward<Src>(in));
}

template <typename Src>
ENDIANIO_INLINE ENDIANIO_CONSTEXPR static auto Read1s(Src&& in) {
	return detail::EndianBaseImpl::Read<detail::endian::native, int8_t>(
			std::forward<Src>(in));
}

template <typename Dst>
ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write1(Dst&& out, uint8_t val) {
	detail::EndianBaseImpl::Write<detail::endian::native>(
			std::forward<Dst>(out), val);
}

template <typename Dst>
ENDIANIO_INLINE ENDIANIO_CONSTEXPR static void Write1s(Dst&& out, int8_t val) {
	detail::EndianBaseImpl::Write<detail::endian::native>(
			std::forward<Dst>(out), val);
}

inline int bitcount(unsigned char n) {
#ifdef __GNUG__
	return __builtin_popcount(n);
#elif defined(_MSC_VER)
	return __popcnt(n);
#else
	auto two = [](auto c) {
		return 1U << c;
	};
	auto mask = [&](auto c) {
		return static_cast<uint8_t>(
				std::numeric_limits<uint8_t>::max() / (two(two(c)) + 1U));
	};
	auto count = [&](auto x, auto c) {
		return (x & mask(c)) + ((x >> two(c)) & mask(c));
	};
	// Only works for 8-bit numbers.
	n = static_cast<unsigned char>(count(n, 0));
	n = static_cast<unsigned char>(count(n, 1));
	n = static_cast<unsigned char>(count(n, 2));
	return n;
#endif
}

#undef ENDIANIO_CONSTEXPR
#undef ENDIANIO_INLINE
#undef ENDIANIO_CONST_INLINE
#undef ENDIANIO_PURE_INLINE

#endif    // ENDIANIO_H
