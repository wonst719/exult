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

#ifndef LIB_ENDIAN_IO_H
#	define LIB_ENDIAN_IO_H

#	include <algorithm>
#	include <cstdint>
#	include <cstring>
#	include <type_traits>

// Need to define manually for MSVC.
#	if defined(_MSC_VER) || defined(_WIN32)
#		ifndef __ORDER_BIG_ENDIAN__
#			define __ORDER_BIG_ENDIAN__ 4321
#		endif
#		ifndef __ORDER_LITTLE_ENDIAN__
#			define __ORDER_LITTLE_ENDIAN__ 1234
#		endif
#		ifndef __BYTE_ORDER__
#			define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#		endif
#	endif

#	if !defined(__BYTE_ORDER__)                          \
			|| (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__ \
				&& __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__)
#		error "Byte order is neither little endian nor big endian. Do not know how to proceed."
#	endif

#	if defined(_MSC_VER)
#		define ENDIANIO_FORCE_INLINE __forceinline
#	else
#		define ENDIANIO_FORCE_INLINE inline
#	endif

// Some things cannot be constexpr on MSVC compiler.
#	ifdef __GNUG__
#		define ENDIANIO_CONSTEXPR constexpr
#	else
#		define ENDIANIO_CONSTEXPR
#	endif

#	if defined(_MSC_VER)
#		include <stdlib.h>
#		pragma intrinsic(_byteswap_ushort)
#		pragma intrinsic(_byteswap_ulong)
#		pragma intrinsic(_byteswap_uint64)
#	elif defined(__INTEL_COMPILER)
#		include <byteswap.h>
#	endif

namespace {    // anonymous

	namespace detail {
		template <typename T>
		using is_an_integer_t = std::enable_if_t<
				std::is_integral<T>::value && !std::is_same<T, bool>::value,
				bool>;

		template <typename T>
		struct is_byte_type_impl : std::integral_constant<bool, false> {};

		template <>
		struct is_byte_type_impl<char> : std::integral_constant<bool, true> {};

		template <>
		struct is_byte_type_impl<unsigned char>
				: std::integral_constant<bool, true> {};

#	if __cplusplus >= 201703L
		template <>
		struct is_byte_type_impl<std::byte>
				: std::integral_constant<bool, true> {};
#	endif

		template <typename T>
		using is_byte_type_t = std::enable_if_t<
				is_byte_type_impl<std::remove_cv_t<T>>::value, bool>;

		enum class endian {
			big    = __ORDER_BIG_ENDIAN__,
			little = __ORDER_LITTLE_ENDIAN__,
			native = __BYTE_ORDER__
		};

		[[gnu::const,
		  gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR uint8_t
				bswap(uint8_t val) noexcept {
			return val;
		}

		[[gnu::const,
		  gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR uint16_t
				bswap(uint16_t val) noexcept {
#	if defined(__GNUG__)
#		ifdef __INTEL_COMPILER
			if (__builtin_constant_p(val)) {
				return __bswap_constant_16(val);
			}
#		endif
			return __builtin_bswap16(val);
#	elif defined(_MSC_VER)
			return _byteswap_ushort(val);
#	else
			return ((val & 0xffu) << 8) | ((val >> 8) & 0xffu);
#	endif
		}

		[[gnu::const,
		  gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR uint32_t
				bswap(uint32_t val) noexcept {
#	if defined(__GNUG__)
#		ifdef __INTEL_COMPILER
			if (__builtin_constant_p(val)) {
				return __bswap_constant_32(val);
			}
#		endif
			return __builtin_bswap32(val);
#	elif defined(_MSC_VER)
			return _byteswap_ulong(val);
#	else
			val = ((val & 0xffffu) << 16) | ((val >> 16) & 0xffffu);
			return ((val & 0xff00ffu) << 8) | ((val >> 8) & 0xff00ffu);
#	endif
		}

		[[gnu::const,
		  gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR uint64_t
				bswap(uint64_t val) noexcept {
#	if defined(__GNUG__)
#		ifdef __INTEL_COMPILER
			if (__builtin_constant_p(val)) {
				return __bswap_constant_64(val);
			}
#		endif
			return __builtin_bswap64(val);
#	elif defined(_MSC_VER)
			return _byteswap_uint64(val);
#	else
			val = ((val & 0xffffffffull) << 32) | ((val >> 32) & 0xffffffffull);
			val = ((val & 0xffff0000ffffull) << 16)
				  | ((val >> 16) & 0xffff0000ffffull);
			return ((val & 0xff00ff00ff00ffull) << 8)
				   | ((val >> 8) & 0xff00ff00ff00ffull);
#	endif
		}

		/**
		 * Functor to convert an integer from @p from_endian
		 * to @p to_endian.
		 */
		template <endian from_endian, endian to_endian>
		struct endian_convert {
			template <typename Int, is_an_integer_t<Int> = true>
			[[gnu::const,
			  gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR Int
					operator()(Int val) const {
				// Being very pedantic here.
				using UInt = typename std::make_unsigned_t<Int>;
				static_assert(
						sizeof(Int) == sizeof(UInt),
						"No suitably sized unsigned integer is available.");
				UInt uval;
				std::memcpy(&uval, &val, sizeof(Int));
				if (from_endian != to_endian) {
					uval = bswap(uval);
				}
				Int ret;
				std::memcpy(&ret, &uval, sizeof(Int));
				return ret;
			}
		};

		template <endian type>
		struct any_endian {
			constexpr static const endian_convert<type, endian::native>
					from_native{};
			constexpr static const endian_convert<endian::native, type>
					to_native{};
			constexpr static const endian_convert<type, endian::big> from_big{};
			constexpr static const endian_convert<endian::big, type> to_big{};
			constexpr static const endian_convert<type, endian::little>
					from_little{};
			constexpr static const endian_convert<endian::little, type>
					to_little{};
		};

		using big_endian    = any_endian<endian::big>;
		using little_endian = any_endian<endian::little>;
		using native_endian = any_endian<endian::native>;

		template <typename Int, endian FromEndian>
		struct read_endian_int {
			template <
					typename Byte, is_an_integer_t<Int> = true,
					is_byte_type_t<Byte> = true>
			[[gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR Int
					operator()(Byte*&& source) const {
				const any_endian<FromEndian> convert;
				Int                          val;
				std::memcpy(&val, source, sizeof(Int));
				return convert.to_native(val);
			}

			template <
					typename Byte, is_an_integer_t<Int> = true,
					is_byte_type_t<Byte> = true>
			[[gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR Int
					operator()(Byte*& source) const {
				const any_endian<FromEndian> convert;
				Int                          val;
				std::memcpy(&val, source, sizeof(Int));
				std::advance(source, sizeof(Int));
				return convert.to_native(val);
			}
		};

		template <typename Int, endian ToEndian>
		struct write_endian_int {
			template <
					typename Byte, is_an_integer_t<Int> = true,
					is_byte_type_t<Byte> = true>
			[[gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR void
					operator()(Byte*&& dest, Int val) const {
				const any_endian<ToEndian> convert;
				Int                        valcvt = convert.from_native(val);
				std::memcpy(dest, &valcvt, sizeof(Int));
			}

			template <
					typename Byte, is_an_integer_t<Int> = true,
					is_byte_type_t<Byte> = true>
			[[gnu::always_inline]] ENDIANIO_FORCE_INLINE ENDIANIO_CONSTEXPR void
					operator()(Byte*& dest, Int val) const {
				const any_endian<ToEndian> convert;
				Int                        valcvt = convert.from_native(val);
				std::memcpy(dest, &valcvt, sizeof(Int));
				std::advance(dest, sizeof(Int));
			}
		};

		template <endian type>
		struct endian_base {
			constexpr static const read_endian_int<uint8_t, type>    Read1{};
			constexpr static const read_endian_int<int8_t, type>     Read1s{};
			constexpr static const read_endian_int<uint16_t, type>   Read2{};
			constexpr static const read_endian_int<int16_t, type>    Read2s{};
			constexpr static const read_endian_int<uint32_t, type>   Read4{};
			constexpr static const read_endian_int<int32_t, type>    Read4s{};
			constexpr static const read_endian_int<uint64_t, type>   Read8{};
			constexpr static const read_endian_int<int64_t, type>    Read8s{};
			constexpr static const read_endian_int<uintptr_t, type>  ReadPtr{};
			constexpr static const write_endian_int<uint8_t, type>   Write1{};
			constexpr static const write_endian_int<int8_t, type>    Write1s{};
			constexpr static const write_endian_int<uint16_t, type>  Write2{};
			constexpr static const write_endian_int<int16_t, type>   Write2s{};
			constexpr static const write_endian_int<uint32_t, type>  Write4{};
			constexpr static const write_endian_int<int32_t, type>   Write4s{};
			constexpr static const write_endian_int<uint64_t, type>  Write8{};
			constexpr static const write_endian_int<int64_t, type>   Write8s{};
			constexpr static const write_endian_int<uintptr_t, type> WritePtr{};
		};
}}    // namespace ::detail

using big_endian    = detail::endian_base<detail::endian::big>;
using little_endian = detail::endian_base<detail::endian::little>;
using native_endian = detail::endian_base<detail::endian::native>;

#	undef ENDIANIO_CONSTEXPR
#	undef ENDIANIO_FORCE_INLINE
#	undef ENDIANIO_LITTLE_ENDIAN

#endif    // LIB_ENDIAN_IO_H

#include <cstdint>

namespace native {
	using Endian = native_endian;

	[[gnu::const]] inline uint8_t testR1(uint8_t*& src) {
		return Endian::Read1(src);
	}

	[[gnu::const]] inline uint16_t testR2(uint8_t*& src) {
		return Endian::Read2(src);
	}

	[[gnu::const]] inline uint32_t testR4(uint8_t*& src) {
		return Endian::Read4(src);
	}

	[[gnu::const]] inline uint64_t testR8(uint8_t*& src) {
		return Endian::Read8(src);
	}

	[[gnu::const]] inline uintptr_t testRPtr(uint8_t*& src) {
		return Endian::ReadPtr(src);
	}

	inline void testW1(uint8_t*& dst, uint8_t val) {
		Endian::Write1(dst, val);
	}

	inline void testW2(uint8_t*& dst, uint16_t val) {
		Endian::Write2(dst, val);
	}

	inline void testW4(uint8_t*& dst, uint32_t val) {
		Endian::Write4(dst, val);
	}

	inline void testW8(uint8_t*& dst, uint64_t val) {
		Endian::Write8(dst, val);
	}

	inline void testWPtr(uint8_t*& dst, uintptr_t val) {
		Endian::WritePtr(dst, val);
	}
}    // namespace native

namespace little {
	using Endian = little_endian;

	[[gnu::const]] inline uint8_t testR1(uint8_t*& src) {
		return Endian::Read1(src);
	}

	[[gnu::const]] inline uint16_t testR2(uint8_t*& src) {
		return Endian::Read2(src);
	}

	[[gnu::const]] inline uint32_t testR4(uint8_t*& src) {
		return Endian::Read4(src);
	}

	[[gnu::const]] inline uint64_t testR8(uint8_t*& src) {
		return Endian::Read8(src);
	}

	[[gnu::const]] inline uintptr_t testRPtr(uint8_t*& src) {
		return Endian::ReadPtr(src);
	}

	inline void testW1(uint8_t*& dst, uint8_t val) {
		Endian::Write1(dst, val);
	}

	inline void testW2(uint8_t*& dst, uint16_t val) {
		Endian::Write2(dst, val);
	}

	inline void testW4(uint8_t*& dst, uint32_t val) {
		Endian::Write4(dst, val);
	}

	inline void testW8(uint8_t*& dst, uint64_t val) {
		Endian::Write8(dst, val);
	}

	inline void testWPtr(uint8_t*& dst, uintptr_t val) {
		Endian::WritePtr(dst, val);
	}
}    // namespace little

namespace big {
	using Endian = big_endian;

	[[gnu::const]] inline uint8_t testR1(uint8_t*& src) {
		return Endian::Read1(src);
	}

	[[gnu::const]] inline uint16_t testR2(uint8_t*& src) {
		return Endian::Read2(src);
	}

	[[gnu::const]] inline uint32_t testR4(uint8_t*& src) {
		return Endian::Read4(src);
	}

	[[gnu::const]] inline uint64_t testR8(uint8_t*& src) {
		return Endian::Read8(src);
	}

	[[gnu::const]] inline uintptr_t testRPtr(uint8_t*& src) {
		return Endian::ReadPtr(src);
	}

	inline void testW1(uint8_t*& dst, uint8_t val) {
		Endian::Write1(dst, val);
	}

	inline void testW2(uint8_t*& dst, uint16_t val) {
		Endian::Write2(dst, val);
	}

	inline void testW4(uint8_t*& dst, uint32_t val) {
		Endian::Write4(dst, val);
	}

	inline void testW8(uint8_t*& dst, uint64_t val) {
		Endian::Write8(dst, val);
	}

	inline void testWPtr(uint8_t*& dst, uintptr_t val) {
		Endian::WritePtr(dst, val);
	}
}    // namespace big
