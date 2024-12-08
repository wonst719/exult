/*
 *  shapevga.cc - Handle the 'shapes.vga' file and associated info.
 *
 *  Copyright (C) 1999  Jeffrey S. Freedman
 *  Copyright (C) 2000-2022  The Exult Team
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

#include "shapevga.h"

#include "ammoinf.h"
#include "aniinf.h"
#include "armorinf.h"
#include "bodyinf.h"
#include "continf.h"
#include "data/exult_bg_flx.h"
#include "data/exult_si_flx.h"
#include "data_utils.h"
#include "effhpinf.h"
#include "exceptions.h"
#include "expinf.h"
#include "frflags.h"
#include "frnameinf.h"
#include "frusefun.h"
#include "headers/polymorphic_value.h"
#include "ignore_unused_variable_warning.h"
#include "lightinf.h"
#include "monstinf.h"
#include "npcdollinf.h"
#include "objdollinf.h"
#include "ready.h"
#include "sfxinf.h"
#include "utils.h"
#include "warminf.h"
#include "weaponinf.h"

using std::ifstream;
using std::ios;
using namespace std;

// For convienience
#define patch_exists(p) (have_patch_path && U7exists(p))
#define patch_name(p)   (patch_exists(p) ? (p) : nullptr)

/*
 *  Open, but don't quit if editing.  We first try the patch name if it's
 *  given.
 */

static std::unique_ptr<std::istream> U7open2(
		const char* pname,    // Patch name, or null.
		const char* fname,    // File name.
		bool        editing) {
	ignore_unused_variable_warning(editing);
	if (pname) {
		return U7open_in(pname);
	}
	try {
		return U7open_in(fname);
	} catch (const file_exception& /*f*/) {
	}
	return nullptr;
}

// Special case ID reader functors.

extern int get_skinvar(const std::string& key);

// Multiracial support in Exult.
class Paperdoll_npc_ID_reader {
public:
	int operator()(std::istream& in, int index, int version, bool binary) {
		ignore_unused_variable_warning(index, version, binary);
		if (in.peek() == '%') {
			const std::string key = ReadStr(in);
			// We need these for Exult, but not for ES.
			// For now, a compromise/hack in that ES defines
			// a version of this function which always returns
			// -1, while Exult has another which forwards to
			// Shapeinfo_lookup::get_skinvar
			const int id = get_skinvar(key);
			return id < 0 ? -1 : id;
		} else {
			return ReadInt(in);
		}
	}
};

// For backward compatibility.
class Body_ID_reader {
public:
	int operator()(std::istream& in, int index, int version, bool binary) {
		ignore_unused_variable_warning(binary);
		return version == 1 ? index : ReadInt(in);
	}
};

// Special case reader functor.
class Gump_reader_functor {
public:
	bool operator()(
			std::istream& in, int version, bool patch, Exult_Game game,
			Shape_info& info) {
		ignore_unused_variable_warning(patch, game);
		info.gump_shape = little_endian::Read2(in);
		if (version >= 2) {
			info.gump_font = little_endian::Read2(in);
		} else {
			info.gump_font = -1;
		}
		return true;
	}
};

// A few custom post-read functors.
class Ready_type_functor {
	Patch_flags_functor<ready_type_flag, Shape_info> setflags;

public:
	void operator()(
			std::istream& in, int version, bool patch, Exult_Game game,
			Shape_info& info) {
		unsigned char ready = info.ready_type;
		info.spell_flag     = ready & 1;
		ready >>= 3;
		const unsigned char spot = game == BLACK_GATE
										   ? Ready_spot_from_BG(ready)
										   : Ready_spot_from_SI(ready);
		info.ready_type          = spot;
		// Init alternate spots.
		switch (spot) {
		case lfinger:
			info.alt_ready1 = rfinger;
			break;
		case lhand:
			info.alt_ready1 = rhand;
			info.alt_ready2 = belt;
			break;
		case both_hands:
			info.alt_ready1 = back_2h;
			break;
		}
		setflags(in, version, patch, game, info);
	}
};

class Actor_flags_functor {
	Patch_flags_functor<actor_flags_flag, Shape_info> setflags;

public:
	void operator()(
			std::istream& in, int version, bool patch, Exult_Game game,
			Shape_info& info) {
		setflags(in, version, patch, game, info);
	}
};

class Paperdoll_npc_functor {
public:
	void operator()(
			std::istream& in, int version, bool patch, Exult_Game game,
			Shape_info& info) {
		ignore_unused_variable_warning(patch, game);
		if (version < 3) {
			// We need this for backward compatibility.
			// We use the setter methods so that the info
			// will get saved by ES if that is needed.
			info.set_gump_data(ReadInt(in, -1), -1);
		}
	}
};

/*
 *  Reads text data file and parses it according to passed
 *  parser functions.
 */
void Read_text_data_file(
		// Name of file to read, sans extension
		const char* fname,
		// What to use to parse data.
		const tcb::span<polymorphic_value<Base_reader>>& parsers,
		// The names of the sections
		const tcb::span<std::string_view>& sections, bool editing,
		Exult_Game game, int resource) {
	auto value_or = [](std::optional<int> opt) {
		return opt ? *opt : 1;
	};
	Text_msg_file_reader static_reader = [&]() {
		if (game == BLACK_GATE || game == SERPENT_ISLE) {
			/*  ++++ Not because of ES.
			snprintf(buf, sizeof(buf), "config/%s", fname);
			str_int_pair resource = game->get_resource(buf);
			U7object txtobj(resource.str, resource.num);
			*/
			const bool  bg = game == BLACK_GATE;
			const char* flexfile
					= bg ? BUNDLE_CHECK(BUNDLE_EXULT_BG_FLX, EXULT_BG_FLX)
						 : BUNDLE_CHECK(BUNDLE_EXULT_SI_FLX, EXULT_SI_FLX);
			IExultDataSource ds(flexfile, resource);
			return Text_msg_file_reader(ds);
		}
		std::string     file = "<STATIC>/" + std::string(fname) + ".txt";
		IFileDataSource ds(file, false);
		if (!ds.good()) {
			throw file_open_exception(file);
		}
		return Text_msg_file_reader(ds);
	}();
	Text_msg_file_reader patch_reader = [&]() {
		std::string file = "<PATCH>/" + std::string(fname) + ".txt";
		if (!U7exists(file)) {
			return Text_msg_file_reader();
		}
		IFileDataSource ds(file, false);
		if (!ds.good()) {
			if (!editing) {
				throw file_open_exception(file);
			}
			return Text_msg_file_reader();
		}
		return Text_msg_file_reader(ds);
	}();

	int static_version = value_or(static_reader.get_version());
	int patch_version  = value_or(patch_reader.get_version());
	std::vector<std::string> strings;

	for (size_t i = 0; i < sections.size(); i++) {
		static_reader.get_section_strings(sections[i], strings);
		parsers[i]->parse(strings, static_version, false, game);
		patch_reader.get_section_strings(sections[i], strings);
		parsers[i]->parse(strings, patch_version, true, game);
	}
}

void Shapes_vga_file::Read_Shapeinf_text_data_file(
		bool editing, Exult_Game game_type) {
	std::array sections{
			"explosions"sv,
			"shape_sfx"sv,
			"animation"sv,
			"usecode_events"sv,
			"mountain_tops"sv,
			"monster_food"sv,
			"actor_flags"sv,
			"effective_hps"sv,
			"lightweight_object"sv,
			"light_data"sv,
			"warmth_data"sv,
			"quantity_frames"sv,
			"locked_containers"sv,
			"content_rules"sv,
			"volatile_explosive"sv,
			"framenames"sv,
			"altready"sv,
			"barge_type"sv,
			"frame_powers"sv,
			"is_jawbone"sv,
			"is_mirror"sv,
			"on_fire"sv,
			"extradimensional_storage"sv,
			"field_type"sv,
			"frame_usecode"sv};
	// For explosions.
	using Explosion_reader = Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<
					Explosion_info, Shape_info, &Shape_info::explosion>>;
	// For sound effects.
	using SFX_reader = Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<SFX_info, Shape_info, &Shape_info::sfxinf>>;
	// For animations.
	using Animation_reader = Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<
					Animation_info, Shape_info, &Shape_info::aniinf>>;
	// For usecode events.
	using Usecode_events_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::usecode_events>,
			Patch_flags_functor<usecode_events_flag, Shape_info>>;
	// For mountain tops.
	using Mountain_top_reader = Functor_multidata_reader<
			Shape_info,
			Text_reader_functor<
					unsigned char, Shape_info, &Shape_info::mountain_top>,
			Patch_flags_functor<mountain_top_flag, Shape_info>>;
	// For monster food.
	using Monster_food_reader = Functor_multidata_reader<
			Shape_info,
			Text_reader_functor<short, Shape_info, &Shape_info::monster_food>,
			Patch_flags_functor<monster_food_flag, Shape_info>>;
	// For actor flags.
	using Actor_flags_reader = Functor_multidata_reader<
			Shape_info,
			Bit_field_text_reader_functor<
					unsigned char, Shape_info, &Shape_info::actor_flags>,
			Actor_flags_functor>;
	// For effective HPs.
	using Effective_hp_reader = Functor_multidata_reader<
			Shape_info,
			Vector_reader_functor<
					Effective_hp_info, Shape_info, &Shape_info::hpinf>>;
	// For lightweight objects.
	using Lightweight_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::lightweight>,
			Patch_flags_functor<lightweight_flag, Shape_info>>;
	// For light data.
	using Light_data_reader = Functor_multidata_reader<
			Shape_info, Vector_reader_functor<
								Light_info, Shape_info, &Shape_info::lightinf>>;
	// For warmth data.
	using Warmth_data_reader = Functor_multidata_reader<
			Shape_info, Vector_reader_functor<
								Warmth_info, Shape_info, &Shape_info::warminf>>;
	// For quantity frames.
	using Quantity_frames_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::quantity_frames>,
			Patch_flags_functor<quantity_frames_flag, Shape_info>>;
	// For locked objects.
	using Locked_containers_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::locked>,
			Patch_flags_functor<locked_flag, Shape_info>>;
	// For content rules.
	using Content_rules_reader = Functor_multidata_reader<
			Shape_info,
			Vector_reader_functor<
					Content_rules, Shape_info, &Shape_info::cntrules>>;
	// For highly explosive objects.
	using Explosive_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::is_volatile>,
			Patch_flags_functor<is_volatile_flag, Shape_info>>;
	// For frame names.
	using Frame_names_reader = Functor_multidata_reader<
			Shape_info,
			Vector_reader_functor<
					Frame_name_info, Shape_info, &Shape_info::nameinf>>;
	// For alternate ready spots.
	using Altready_reader = Functor_multidata_reader<
			Shape_info,
			Text_pair_reader_functor<
					unsigned char, Shape_info, &Shape_info::alt_ready1,
					&Shape_info::alt_ready2>,
			Patch_flags_functor<altready_type_flag, Shape_info>>;
	// For barge parts.
	using Barge_type_reader = Functor_multidata_reader<
			Shape_info,
			Text_reader_functor<
					unsigned char, Shape_info, &Shape_info::barge_type>,
			Patch_flags_functor<barge_type_flag, Shape_info>>;
	// For frame flags.
	using Frame_flags_reader = Functor_multidata_reader<
			Shape_info,
			Vector_reader_functor<
					Frame_flags_info, Shape_info, &Shape_info::frflagsinf>>;
	// For the jawbone.
	using Jawbone_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::jawbone>,
			Patch_flags_functor<jawbone_flag, Shape_info>>;
	// Mirrors.
	using Mirror_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::mirror>,
			Patch_flags_functor<mirror_flag, Shape_info>>;
	// Objects on fire.
	using On_fire_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::on_fire>,
			Patch_flags_functor<on_fire_flag, Shape_info>>;
	// Containers with unlimited storage.
	using Extradimensional_storage_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::extradimensional_storage>,
			Patch_flags_functor<extradimensional_storage_flag, Shape_info>>;
	// For field types.
	using Field_type_reader = Functor_multidata_reader<
			Shape_info,
			Text_reader_functor<
					signed char, Shape_info, &Shape_info::field_type>,
			Patch_flags_functor<field_type_flag, Shape_info>>;
	// For frame usecode.
	using Frame_usecode_reader = Functor_multidata_reader<
			Shape_info,
			Vector_reader_functor<
					Frame_usecode_info, Shape_info, &Shape_info::frucinf>>;

	std::array readers = {
			make_polymorphic_value<Base_reader, Explosion_reader>(info),
			make_polymorphic_value<Base_reader, SFX_reader>(info),
			make_polymorphic_value<Base_reader, Animation_reader>(info),
			make_polymorphic_value<Base_reader, Usecode_events_reader>(info),
			make_polymorphic_value<Base_reader, Mountain_top_reader>(info),
			make_polymorphic_value<Base_reader, Monster_food_reader>(info),
			make_polymorphic_value<Base_reader, Actor_flags_reader>(info),
			make_polymorphic_value<Base_reader, Effective_hp_reader>(info),
			make_polymorphic_value<Base_reader, Lightweight_reader>(info),
			make_polymorphic_value<Base_reader, Light_data_reader>(info),
			make_polymorphic_value<Base_reader, Warmth_data_reader>(info),
			make_polymorphic_value<Base_reader, Quantity_frames_reader>(info),
			make_polymorphic_value<Base_reader, Locked_containers_reader>(info),
			make_polymorphic_value<Base_reader, Content_rules_reader>(info),
			make_polymorphic_value<Base_reader, Explosive_reader>(info),
			make_polymorphic_value<Base_reader, Frame_names_reader>(info),
			make_polymorphic_value<Base_reader, Altready_reader>(info),
			make_polymorphic_value<Base_reader, Barge_type_reader>(info),
			make_polymorphic_value<Base_reader, Frame_flags_reader>(info),
			make_polymorphic_value<Base_reader, Jawbone_reader>(info),
			make_polymorphic_value<Base_reader, Mirror_reader>(info),
			make_polymorphic_value<Base_reader, On_fire_reader>(info),
			make_polymorphic_value<
					Base_reader, Extradimensional_storage_reader>(info),
			make_polymorphic_value<Base_reader, Field_type_reader>(info),
			make_polymorphic_value<Base_reader, Frame_usecode_reader>(info),
	};
	static_assert(sections.size() == readers.size());
	const int flxres = game_type == BLACK_GATE ? EXULT_BG_FLX_SHAPE_INFO_TXT
											   : EXULT_SI_FLX_SHAPE_INFO_TXT;

	Read_text_data_file(
			"shape_info", readers, sections, editing, game_type, flxres);
}

void Shapes_vga_file::Read_Bodies_text_data_file(
		bool editing, Exult_Game game_type) {
	std::array sections{"bodyshapes"sv, "bodylist"sv};
	using Body_shape_reader = Functor_multidata_reader<
			Shape_info,
			Bit_text_reader_functor<
					unsigned short, Shape_info, &Shape_info::shape_flags,
					Shape_info::is_body>,
			Patch_flags_functor<is_body_flag, Shape_info>, Body_ID_reader>;
	using Body_list_reader = Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<Body_info, Shape_info, &Shape_info::body>>;
	std::array readers{
			make_polymorphic_value<Base_reader, Body_shape_reader>(info),
			make_polymorphic_value<Base_reader, Body_list_reader>(info)};
	static_assert(sections.size() == readers.size());
	const int flxres = game_type == BLACK_GATE ? EXULT_BG_FLX_BODIES_TXT
											   : EXULT_SI_FLX_BODIES_TXT;

	Read_text_data_file(
			"bodies", readers, sections, editing, game_type, flxres);
}

void Shapes_vga_file::Read_Paperdoll_text_data_file(
		bool editing, Exult_Game game_type) {
	std::array sections{"characters"sv, "items"sv};
	using NpcPaperdoll_reader_functor = Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<
					Paperdoll_npc, Shape_info, &Shape_info::npcpaperdoll>,
			Paperdoll_npc_functor, Paperdoll_npc_ID_reader>;
	using ObjPaperdoll_reader_functor = Functor_multidata_reader<
			Shape_info,
			Vector_reader_functor<
					Paperdoll_item, Shape_info, &Shape_info::objpaperdoll>>;
	std::array readers{
			make_polymorphic_value<Base_reader, NpcPaperdoll_reader_functor>(
					info),
			make_polymorphic_value<Base_reader, ObjPaperdoll_reader_functor>(
					info),
	};
	static_assert(sections.size() == readers.size());
	const int flxres = game_type == BLACK_GATE ? EXULT_BG_FLX_PAPERDOL_INFO_TXT
											   : EXULT_SI_FLX_PAPERDOL_INFO_TXT;

	Read_text_data_file(
			"paperdol_info", readers, sections, editing, game_type, flxres);
}

/*
 *  Reload static data for weapons, ammo and mosters to
 *  fix data that was lost by earlier versions of ES.
 */
void Shapes_vga_file::fix_old_shape_info(Exult_Game game    // Which game.
) {
	if (!info_read) {    // Read info first.
		read_info(game, true);
	}
	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<Weapon_info, Shape_info, &Shape_info::weapon>>
			weapon(info);
	weapon.read(WEAPONS, false, game);
	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<Ammo_info, Shape_info, &Shape_info::ammo>>
			ammo(info);
	ammo.read(AMMO, false, game);
	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<
					Monster_info, Shape_info, &Shape_info::monstinf>>
			monstinf(info);
	monstinf.read(MONSTERS, false, game);
}

/*
 *  Reload info.
 */

void Shapes_vga_file::reload_info(Exult_Game game    // Which game.
) {
	info_read = false;
	info.clear();
	read_info(game);
}

/*
 *  Read in data files about shapes.
 *
 *  Output: 0 if error.
 */

bool Shapes_vga_file::read_info(
		Exult_Game game,      // Which game.
		bool       editing    // True to allow files to not exist.
) {
	if (info_read) {
		return false;
	}
	info_read                  = true;
	const bool have_patch_path = is_system_path_defined("<PATCH>");

	// ShapeDims

	// Starts at 0x96'th shape.
	auto pShpdims = U7open2(patch_name(PATCH_SHPDIMS), SHPDIMS, editing);
	if (pShpdims) {
		auto& shpdims = *pShpdims;
		for (size_t i = c_first_obj_shape; i < shapes.size() && !shpdims.eof();
			 i++) {
			info[i].shpdims[0] = shpdims.get();
			info[i].shpdims[1] = shpdims.get();
		}
	}

	// WGTVOL
	auto pWgtvol = U7open2(patch_name(PATCH_WGTVOL), WGTVOL, editing);
	if (pWgtvol) {
		auto& wgtvol = *pWgtvol;
		for (size_t i = 0; i < shapes.size() && !wgtvol.eof(); i++) {
			info[i].weight = wgtvol.get();
			info[i].volume = wgtvol.get();
		}
	}

	// TFA
	auto pTfa = U7open2(patch_name(PATCH_TFA), TFA, editing);
	if (pTfa) {
		auto& tfa = *pTfa;
		for (size_t i = 0; i < shapes.size() && !tfa.eof(); i++) {
			tfa.read(reinterpret_cast<char*>(&info[i].tfa[0]), 3);
			info[i].set_tfa_data();
		}
	}

	if (game == BLACK_GATE || game == SERPENT_ISLE) {
		// Animation data at the end of BG and SI TFA.DAT
		// We *should* blow up if TFA not there.
		auto pStfa = U7open_in(TFA);
		if (!pStfa) {
			throw file_open_exception(TFA);
		}
		auto& stfa = *pStfa;
		stfa.seekg(3 * 1024);
		unsigned char buf[512];
		stfa.read(reinterpret_cast<char*>(buf), 512);
		unsigned char* ptr = buf;
		for (int i = 0; i < 512; i++, ptr++) {
			int val   = *ptr;
			int shape = 2 * i;
			while (val) {
				if (val & 0xf) {
					delete info[shape].aniinf;
					info[shape].aniinf = Animation_info::create_from_tfa(
							val & 0xf, get_num_frames(shape));
				}
				val >>= 4;
				shape++;
			}
		}
	}

	// Load data about drawing the weapon in an actor's hand
	auto pWihh = U7open2(patch_name(PATCH_WIHH), WIHH, editing);
	if (pWihh) {
		auto&          wihh = *pWihh;
		const size_t   cnt  = shapes.size();
		unsigned short offsets[c_max_shapes];
		for (size_t i = 0; i < cnt; i++) {
			offsets[i] = little_endian::Read2(wihh);
		}
		for (size_t i = 0; i < cnt; i++) {
			// A zero offset means there is no record
			if (offsets[i] == 0) {
				info[i].weapon_offsets = nullptr;
			} else {
				wihh.seekg(offsets[i]);
				// There are two bytes per frame: 64 total
				info[i].weapon_offsets = new unsigned char[64];
				for (int j = 0; j < 32; j++) {
					unsigned char x = Read1(wihh);
					unsigned char y = Read1(wihh);
					// Set x/y to 255 if weapon is not to be drawn
					// In the file x/y are either 64 or 255:
					// I am assuming that they mean the same
					if (x > 63 || y > 63) {
						x = y = 255;
					}
					info[i].weapon_offsets[j * 2]     = x;
					info[i].weapon_offsets[j * 2 + 1] = y;
				}
			}
		}
	}

	// Read flags from occlude.dat.
	auto pOcc = U7open2(patch_name(PATCH_OCCLUDE), OCCLUDE, editing);
	if (pOcc) {
		auto&         occ = *pOcc;
		unsigned char occbits[c_occsize];    // c_max_shapes bit flags.
		// Ensure sensible defaults.
		memset(&occbits[0], 0, sizeof(occbits));
		occ.read(reinterpret_cast<char*>(occbits), sizeof(occbits));
		for (int i = 0; i < occ.gcount(); i++) {
			unsigned char bits  = occbits[i];
			const int     shnum = i * 8;    // Check each bit.
			for (int b = 0; bits; b++, bits = bits >> 1) {
				if (bits & 1) {
					info[shnum + b].occludes_flag = true;
				}
			}
		}
	}

	// Get 'equip.dat'.
	auto pMfile = U7open2(patch_name(PATCH_EQUIP), EQUIP, editing);
	if (pMfile) {
		auto& mfile = *pMfile;
		// Get # entries (with Exult extension).
		const int num_recs = Read_count(mfile);
		Monster_info::reserve_equip(num_recs);
		for (int i = 0; i < num_recs; i++) {
			Equip_record equip;
			// 10 elements/record.
			for (int elem = 0; elem < 10; elem++) {
				const int      shnum = little_endian::Read2(mfile);
				const unsigned prob  = Read1(mfile);
				const unsigned quant = Read1(mfile);
				little_endian::Read2(mfile);
				equip.set(elem, shnum, prob, quant);
			}
			Monster_info::add_equip(equip);
		}
	}

	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<Armor_info, Shape_info, &Shape_info::armor>>
			armor(info);
	armor.read(ARMOR, false, game);
	armor.read(PATCH_ARMOR, true, game);

	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<Weapon_info, Shape_info, &Shape_info::weapon>>
			weapon(info);
	weapon.read(WEAPONS, false, game);
	weapon.read(PATCH_WEAPONS, true, game);

	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<Ammo_info, Shape_info, &Shape_info::ammo>>
			ammo(info);
	ammo.read(AMMO, false, game);
	ammo.read(PATCH_AMMO, true, game);

	Functor_multidata_reader<
			Shape_info,
			Class_reader_functor<
					Monster_info, Shape_info, &Shape_info::monstinf>>
			monstinf(info);
	monstinf.read(MONSTERS, false, game);
	monstinf.read(PATCH_MONSTERS, true, game);

	Functor_multidata_reader<
			Shape_info, Gump_reader_functor,
			Patch_flags_functor<gump_shape_flag, Shape_info>>
			gump(info, true);
	if (game == BLACK_GATE || game == SERPENT_ISLE) {
		gump.read(
				game, game == BLACK_GATE ? EXULT_BG_FLX_CONTAINER_DAT
										 : EXULT_SI_FLX_CONTAINER_DAT);
	} else {
		gump.read(CONTAINER, false, game);
	}
	gump.read(PATCH_CONTAINER, true, game);

	Functor_multidata_reader<
			Shape_info,
			Binary_reader_functor<
					unsigned char, Shape_info, &Shape_info::ready_type, 6>,
			Ready_type_functor>
			ready(info);
	ready.read(READY, false, game);
	ready.read(PATCH_READY, true, game);

	Read_Shapeinf_text_data_file(editing, game);
	Read_Bodies_text_data_file(editing, game);
	Read_Paperdoll_text_data_file(editing, game);

	// Ensure valid ready spots for all shapes.
	const unsigned char defready = game == BLACK_GATE ? backpack : rhand;
	zinfo.ready_type             = defready;
	for (auto& it : info) {
		Shape_info& inf = it.second;
		if (inf.ready_type == invalid_spot) {
			inf.ready_type = defready;
		}
	}
	bool auto_modified = false;
	for (auto& it : info) {
		const int   shnum = it.first;
		Shape_info& inf   = it.second;
		if (inf.has_monster_info()) {
			Monster_info* minf = inf.monstinf;
			if (minf->can_teleport()) {
				std::cerr << "Shape " << shnum
						  << " is a monster that can teleport, teleport flag "
							 "moved from Monster info ( monster.dat ) to Actor "
							 "info ( shape_info.txt ) as NPC flags."
						  << std::endl;
				inf.set_actor_flag(Shape_info::teleports);
				minf->set_can_teleport(false);
				auto_modified = true;
			}
			if (minf->can_summon()) {
				std::cerr << "Shape " << shnum
						  << " is a monster that can summon, summon flag moved "
							 "from Monster info ( monster.dat ) to Actor info "
							 "( shape_info.txt ) as NPC flags."
						  << std::endl;
				inf.set_actor_flag(Shape_info::summons);
				minf->set_can_summon(false);
				auto_modified = true;
			}
			if (minf->can_be_invisible()) {
				std::cerr << "Shape " << shnum
						  << " is a monster that can be invisible, invisible "
							 "flag moved from Monster info ( monster.dat ) to "
							 "Actor info ( shape_info.txt ) as NPC flags."
						  << std::endl;
				inf.set_actor_flag(Shape_info::turns_invisible);
				minf->set_can_be_invisible(false);
				auto_modified = true;
			}
		}
	}
	return auto_modified;
}

/*
 *  Open/close file.
 */

Shapes_vga_file::Shapes_vga_file(
		const char* nm,        // Path to file.
		int         u7drag,    // # from u7drag.h, or -1.
		const char* nm2        // Path to patch version, or 0.
		)
		: Vga_file(nm, u7drag, nm2) {}

void Shapes_vga_file::init() {
	if (is_system_path_defined("<PATCH>") && U7exists(PATCH_SHAPES)) {
		load(SHAPES_VGA, PATCH_SHAPES);
	} else {
		load(SHAPES_VGA);
	}
	info_read = false;
}

/*
 *  Make a spot for a new shape, and delete frames in existing shape.
 *
 *  Output: ->shape, or 0 if invalid shapenum.
 */

Shape* Shapes_vga_file::new_shape(int shapenum) {
	Shape* newshape = Vga_file::new_shape(shapenum);
	return newshape;
}
