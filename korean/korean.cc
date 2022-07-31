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

#include "korean.h"

#include "ibuf8.h"
#include "files/databuf.h"

struct KoreanGlyph {
	int xOffset;
	int yOffset;
	int advance;
	byte* glyphPtr;
};

KoreanFont::KoreanFont() : _pal() {
}

bool KoreanFont::load(const std::string& fontName) {
	IFileDataSource ifs(File_spec(fontName.c_str()));

	uint16 glyphCount;
	uint32 magic = ifs.read4();
	if (magic == 0x11223345) {
		_width = ifs.read1();
		_height = ifs.read1();
		_bitmapWidth = ifs.read1();
		_bitmapHeight = ifs.read1();

		ifs.read(_pal, 4);

		glyphCount = ifs.read2();

		int glyphLen = (int)(_bitmapWidth * _bitmapHeight) / 4;
		_fontPtr = new byte[glyphLen * glyphCount];

		for (int i = 0; i < glyphCount; i++) {
			uint16 codepoint = ifs.read2();
			int8 xOffset = ifs.read1();
			int8 yOffset = ifs.read1();
			int16 advance = ifs.read1();
			KoreanGlyph* glyph = new KoreanGlyph;
			glyph->xOffset = xOffset;
			glyph->yOffset = yOffset;
			glyph->advance = advance;
			glyph->glyphPtr = _fontPtr + glyphLen * i;
			_glyphMap.emplace(codepoint, glyph);
		}

		ifs.read(_fontPtr, glyphLen * glyphCount);
	} else if (magic == 0x11223344) {
		_width = ifs.read1();
		_height = ifs.read1();
		_bitmapWidth = ifs.read1();
		_bitmapHeight = ifs.read1();
		int8 xOffset = ifs.read1();
		int8 yOffset = ifs.read1();
		int16 advance = ifs.read2();

		ifs.read(_pal, 4);

		glyphCount = ifs.read2();

		int glyphLen = (int)(_bitmapWidth * _bitmapHeight) / 4;
		_fontPtr = new byte[glyphLen * glyphCount];

		for (uint16 i = 0; i < glyphCount; i++) {
			KoreanGlyph* glyph = new KoreanGlyph;
			glyph->xOffset = xOffset;
			glyph->yOffset = yOffset;
			glyph->advance = advance;
			glyph->glyphPtr = _fontPtr + glyphLen * i;
			_glyphMap.emplace(ifs.read2(), glyph);
		}

		ifs.read(_fontPtr, glyphLen * glyphCount);
	} else {
		return false;
	}

	return true;
}

KoreanFont::~KoreanFont() {
	for (auto pair : _glyphMap) {
		delete pair.second;
	}
	_glyphMap.clear();
	delete[] _fontPtr;
}

int KoreanFont::drawGlyph(Image_buffer8* dst, uint16 codepoint, int dx, int dy) {
	if (!_fontPtr)
		return 0;

	auto glyphIter = _glyphMap.find(codepoint);
	if (glyphIter == _glyphMap.end()) {
		return 0;
	}
	KoreanGlyph* glyph = glyphIter->second;
	byte* src = glyph->glyphPtr;

	byte bits = 0;

	dx += glyph->xOffset;
	dy += glyph->yOffset;

	for (int y = 0; y < _bitmapHeight; y++) {
		for (int x = 0; x < _bitmapWidth; x++) {
			if ((x % 4) == 0)
				bits = *src++;
			byte c = (bits >> 6) & 0x3;
			bits <<= 2;
			if (c != 0) {
				dst->put_pixel8(_pal[c], dx + x, dy + y);
			}
		}
	}

	return glyphIter->second->advance;
}

int KoreanFont::getGlyphAdvance(uint16 codepoint) {
	auto iter = _glyphMap.find(codepoint);
	if (iter == _glyphMap.end())
		return 0;
	return iter->second->advance;
}

int KoreanFont::getFontHeight() {
	return _height;
}

bool KoreanFont::hasGlyph(uint16 codepoint) {
	return _glyphMap.find(codepoint) != _glyphMap.end();
}
