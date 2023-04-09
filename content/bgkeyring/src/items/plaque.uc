/*
 *
 *  Copyright (C) 2006  The Exult Team
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
 *
 *
 *	This source file contains usecode for the new plaques.
 *
 *	Author: Marzo Junior
 *	Last Modified: 2006-02-27
 */

void Plaque shape#(0x334) () {
	var qual = get_item_quality();
	if (qual < 100) {
		Plaque.original();
	} else {
		var msg;
		if (qual == 100) {
			msg = ["\{dot}e", "flame", "of", "tru\{dot}"];
		} else if (qual == 101) {
			msg = ["\{dot}e", "flame", "of", "love"];
		} else if (qual == 102) {
			msg = ["\{dot}e", "flame", "of", "courage"];
		} else if (qual == 103) {
			msg = ["\{dot}e", "flame", "of", "infinity"];
		} else if (qual == 104) {
			msg = ["\{dot}e", "flame", "of", "si\{ng}ularity"];
		} else if (qual == 105) {
			msg = ["shrine", "of", "\{dot}e", "codex"];
		} else if (qual == 106) {
			msg = ["ch\{ea}ters", "never", "win"];
		} else if (qual == 107) {
			msg = ["\{dot}e", "book", "of", "tru\{dot}"];
		} else if (qual == 108) {
			msg = ["\{dot}e", "candle", "of", "love"];
		} else if (qual == 109) {
			msg = ["\{dot}e", "bell", "of", "courage"];
		}
		UI_display_runes(0x0033, msg);
	}
}
