/**
 ** Fontvga.cc - Handle the 'fonts.vga' file and text rendering.
 **
 ** Written: 4/29/99 - JSF
 **/
/*
Copyright (C) 1999  Jeffrey S. Freedman
Copyright (C) 2000-2022  The Exult Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "fontvga.h"

#include "Flex.h"
#include "array_size.h"
#include "fnames.h"
#include "utils.h"

#include <cctype>
#include <fstream>
#include <iostream>

// using std::string;

/*
 *  Fonts in 'fonts.vga':
 *
 *  0 = Normal yellow.
 *  1 = Large runes.
 *  2 = small black (as in zstats).
 *  3 = runes.
 *  4 = tiny black, used in books.
 *  5 = little white, glowing, for spellbooks.
 *  6 = runes.
 *  7 = normal red.
 *  8 = Serpentine (books)
 *  9 = Serpentine (signs)
 *  10 = Serpentine (gold signs)
 */

/*
 *  Horizontal leads, by fontnum:
 *
 *  This must include the Endgame fonts (currently 32-35)!!
 *      And the MAINSHP font (36)
 *  However, their values are set elsewhere
 */
// +TODO: This shouldn't be hard-coded.
static int hlead[] = {-2, -1, 0, -1, 0, 0, -1, -2, -1, -1};

/*
 *  Initialize.
 */

void Fonts_vga_file::init() {
	const int cnt = array_size(hlead);

	FlexFile  sfonts(FONTS_VGA);
	FlexFile  pfonts(PATCH_FONTS);
	const int sn       = static_cast<int>(sfonts.number_of_objects());
	const int pn       = static_cast<int>(pfonts.number_of_objects());
	const int numfonts = pn > sn ? pn : sn;
	fonts.resize(numfonts);

	for (int i = 0; i < numfonts; i++) {
		fonts[i].load(FONTS_VGA, PATCH_FONTS, i, i < cnt ? hlead[i] : 0, 0);
	}
}
