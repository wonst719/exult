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

KoreanFont::KoreanFont() : _pal() {
}

bool KoreanFont::load(const std::string& fontName) {
	IFileDataSource ifs(File_spec(fontName.c_str()));

	uint32 magic = ifs.read4();
	_width = ifs.read1();
	_height = ifs.read1();
	_bitmapWidth = ifs.read1();
	_bitmapHeight = ifs.read1();
	_xOffset = ifs.read1();
	_yOffset = ifs.read1();
	_advance = ifs.read2();

	ifs.read(_pal, 4);

	uint16 glyphCount = ifs.read2();

	int glyphLen = (int)(_bitmapWidth * _bitmapHeight) / 4;

	for (uint16 i = 0; i < glyphCount; i++) {
		_glyphOffsetMap.emplace(ifs.read2(), glyphLen * i);
	}

	_fontPtr = new byte[glyphLen * glyphCount];
	ifs.read(_fontPtr, glyphLen * glyphCount);

	return true;
}

KoreanFont::~KoreanFont() {
	delete[] _fontPtr;
}

int KoreanFont::drawGlyph(Image_buffer8* dst, uint16 codepoint, int dx, int dy) {
	if (!_fontPtr)
		return 0;

	auto glyphIter = _glyphOffsetMap.find(codepoint);
	if (glyphIter == _glyphOffsetMap.end()) {
		return 0;
	}
	byte* src = _fontPtr + glyphIter->second;

	byte bits = 0;

	dx += _xOffset;
	dy += _yOffset;

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

	return _advance;
}

int KoreanFont::getGlyphWidth(uint16 codepoint) {
	return _width;
}

int KoreanFont::getFontHeight() {
	return _height;
}
