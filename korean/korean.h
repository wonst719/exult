/*
 *  Copyright (C) 2001  The Exult Team
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
typedef unsigned char byte;

extern int _2byteWidth;
extern int _2byteHeight;

extern bool loadKoreanFont();
extern byte *getKoreanPtr(int idx);
extern void drawKorean(Image_buffer8 *dst, int dx, int dy, byte _color, byte *src);

static inline bool checkKSCode(byte hi, byte lo) {
	//hi : xx
	//lo : yy
	if ((0xA1 > lo) || (0xFE < lo)) {
		return false;
	}
	if ((hi >= 0xB0) && (hi <= 0xC8)) {
		return true;
	}
	return false;
}

// UCS-2 범위에서만 작동함
static inline wchar_t DecodeUtf8Codepoint(const char* text, int& out_len) {
	// Decode UTF-8
	unsigned int codepoint = 0;
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text);
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

static inline wchar_t DecodeUtf8Codepoint(const char* text) {
	int _;
	return DecodeUtf8Codepoint(text, _);
}

#include "korean/ucs2kstable.h"

#endif //KOREAN_H
