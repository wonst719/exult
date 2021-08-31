/*
 * Copyright (C) 2001-2005 wonst719
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "korean.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

#include "ibuf8.h"
#include "files/databuf.h"

KoreanFont::KoreanFont() {
}

bool KoreanFont::load(const std::string& fontName) {
	IFileDataSource ifs(File_spec(fontName.c_str()));

	uint32 magic = ifs.read4();
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

int _2byteWidth = 0;
int _2byteHeight = 0;
byte *_2byteFontPtr = NULL;

int _2byteMultiWidth[20] = {0, };
int _2byteMultiHeight[20] = {0, };
byte *_2byteMultiFontPtr[20] = {NULL, };

bool _useCJKMode = false;
bool _useMultiFont = false;
int _numLoadedFont = 0;

KoreanFont kf;
bool loadKoreanFont() {
	kf.load("FONT.FNT");

	FILE *fp;
	_useCJKMode = false;
	int numChar = 0;
	_useMultiFont = false;
	const char *fontFile = NULL;

	_useMultiFont = true;
	fontFile = "korean.fnt";
	numChar = 2350;

	if (_useMultiFont) {
		fprintf(stdout,"Loading Korean Multi Font System\n");
		_useCJKMode = true;
		_numLoadedFont = 0;

		for (int i = 0; i < 20; i++) {
			char ff[256];
			sprintf(ff, "korean%02d.fnt", i);

			_2byteMultiFontPtr[i] = NULL;

			if ((fp = fopen(ff, "rb"))) {
				_numLoadedFont++;
				fseek(fp, 2, SEEK_CUR);
				_2byteMultiWidth[i] = fgetc(fp);
				_2byteMultiHeight[i] = fgetc(fp);

				int fontSize = ((_2byteMultiWidth[i] + 7) / 8) * _2byteMultiHeight[i] * numChar;
				_2byteMultiFontPtr[i] = new byte[fontSize];
				fprintf(stdout,"#%d, size %d\n", i, fontSize);
				fread(_2byteMultiFontPtr[i], fontSize, 1, fp);
				fclose(fp);
			}

			_2byteFontPtr = NULL;
			_2byteWidth = 0;
			_2byteHeight = 0;
		}
		if (_numLoadedFont == 0) {
			fprintf(stdout,"Cannot load any font\n");
			_useMultiFont = false;
		} else {
			fprintf(stdout,"%d fonts are loaded\n", _numLoadedFont);
		}
	}
	if(!_useMultiFont) {
		fprintf(stdout,"Loading CJK Single Font System\n");
		if ((fp = fopen(fontFile, "rb"))) {
			fprintf(stdout,"Loading Font\n");
			_useCJKMode = true;
			fseek(fp, 2, SEEK_CUR);
			_2byteWidth = fgetc(fp);
			_2byteHeight = fgetc(fp);

			int fontSize = ((_2byteWidth + 7) / 8) * _2byteHeight * numChar;
			_2byteFontPtr = new byte[fontSize];
			fprintf(stdout,"#%d, size %d\n", 0, fontSize);
			fread(_2byteFontPtr, fontSize, 1, fp);
			fclose(fp);
		} else {
			fprintf(stdout,"Couldn't load any font\n");
			return false;
		}
	}
	return true;
}

bool unloadKoreanFiles()
{
	delete []_2byteFontPtr;
	return true;
}

byte *getKoreanPtr(int idx)
{
	idx = ((idx % 256) - 0xb0) * 94 + (idx / 256) - 0xa1;
	if (idx > 2350 || idx < 0) {
		printf("WARNING: 잘못된 한글 인덱스 %d입니다.\n", idx);
		return 0;
	}
 	return _2byteFontPtr + ((_2byteWidth + 7) / 8) * _2byteHeight * idx;
}

static const byte revBitMask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

int drawKorean(Image_buffer8* dst, uint16 cp, int dx, int dy) {
	return kf.drawGlyph(dst, cp, dx, dy);
}

void drawKorean(Image_buffer8 *dst, int dx, int dy, uint8 _color, byte *src) {
	if (!src)
		return;

	int y, x;
	byte bits = 0;

	int height = _2byteHeight, width = _2byteWidth;

	// A bit slow method. but, well, don't care :)
	int offsetX[14] = { -1,  0,  1, -1, 1, -1, 0, 1, 0, 1, 2, 2, 2, 0 };
	int offsetY[14] = { -1, -1, -1,  0, 0,  1, 1, 1, 2, 2, 0, 1, 2, 0 };
	int cTable[14] =  {  0,  0,  0,  0, 0,  0, 0, 0, 0, 0, 0, 0, 0, _color };
	byte *orig_src = src;

	for (int i = 0; i < 14; i++) {
		_color = cTable[i];
		src = orig_src;

		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if ((x % 8) == 0)
					bits = *src++;
				if ((bits & revBitMask[x % 8])) {
					dst->put_pixel8(_color, dx + x + offsetX[i], dy + y + offsetY[i]);
				}
			}
		}
	}
}
