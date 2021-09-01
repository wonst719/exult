/*
 *  Copyright (C) 2021 The Exult Team, wonst719
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

#ifndef KOREAN_H
#define KOREAN_H

#include "ibuf8.h"
#include <map>
#include <string>

typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

class KoreanFont {
	int _width = 0;
	int _height = 0;
	byte* _fontPtr = nullptr;

	std::map<uint16, int> _glyphOffsetMap;

	uint8 _bitmapWidth = 0;
	uint8 _bitmapHeight = 0;
	int8 _xOffset = 0;
	int8 _yOffset = 0;
	int16 _advance = 0;

	uint8 _pal[4];

public:
	KoreanFont();
	~KoreanFont();

	bool load(const std::string& fontName);
	int drawGlyph(Image_buffer8* dst, uint16 codepoint, int dx, int dy);
	int getGlyphWidth(uint16 codepoint);
	int getFontHeight();
};

// UCS-2 범위에서만 작동함
static inline uint32 DecodeUtf8Codepoint(const char* text, int& out_len) {
	// Decode UTF-8
	uint32 codepoint = 0;
	const byte* bytes = reinterpret_cast<const byte*>(text);
	if ((bytes[0] & 0x80) == 0) {
		// 0xxxxxxx
		codepoint = bytes[0];
		out_len = 1;
	} else if ((bytes[0] & 0xE0) == 0xC0 && (bytes[1] & 0xC0) == 0x80) {
		// 110xxxxx 10xxxxxx
		codepoint = (bytes[0] & 0x1F) << 6 | (bytes[1] & 0x3F);
		out_len = 2;
	} else if ((bytes[0] & 0xE0) == 0xE0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80) {
		// 1110xxxx 10xxxxxx 10xxxxxx
		codepoint = (bytes[0] & 0x0F) << 12 | (bytes[1] & 0x3F) << 6 | (bytes[2] & 0x3F);
		out_len = 3;
	}

	return codepoint;
}

static inline uint32 DecodeUtf8Codepoint(const char* text) {
	int _;
	return DecodeUtf8Codepoint(text, _);
}

#include "korean/ucs2kstable.h"

#endif //KOREAN_H
