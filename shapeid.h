/*
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

#ifndef SHAPEID_H
#define SHAPEID_H   1

#include "exult_constants.h"
#include "shapevga.h"
#include "fontvga.h"
#include "singles.h"
#include <vector>
#include <memory>


class Shape_frame;
class Shape_info;
class Font;
class Image_buffer8;
struct Cursor_info;

enum ShapeFile {
    SF_SHAPES_VGA = 0,  // <STATIC>/shapes.vga.  MUST be first.
    SF_GUMPS_VGA,       // <STATIC>/gumps.vga
    SF_PAPERDOL_VGA,    // <STATIC>/paperdol.vga
    SF_SPRITES_VGA,     // <STATIC>/sprites.vga
    SF_FACES_VGA,       // <STATIC>/faces.vga
    SF_EXULT_FLX,       // <DATA>/exult.flx
    SF_GAME_FLX,        // <DATA>/bg_data.flx or <DATA>/si_data.flx
    // Not yet
    //SF_FONTS_VGA,     // <STATIC>/fonts.vga
    SF_OTHER,       // Other unknown FLX
    SF_COUNT        // # of preceding entries.
};

// Special pixels.
enum Pixel_colors {POISON_PIXEL = 0, PROTECT_PIXEL, CURSED_PIXEL,
                   CHARMED_PIXEL, HIT_PIXEL, PARALYZE_PIXEL, BLACK_PIXEL, NPIXCOLORS
                  };

/*
 *  Manage the set of shape files.
 */
class Shape_manager : public Game_singletons {
public:
	struct Cached_shape {
		Shape_frame *shape;
		bool has_trans;
	};

private:
	static Shape_manager *instance; // There shall be only one.
	Shapes_vga_file shapes;     // Main 'shapes.vga' file.
	Vga_file files[static_cast<int>(SF_COUNT)]; // The files we manage.
	std::unique_ptr<Fonts_vga_file> fonts = nullptr;      // "fonts.vga" file.
	std::vector<Xform_palette> xforms;  // Transforms translucent colors
	//   0xf4 through 0xfe.
	Xform_palette *invis_xform; // For showing invisible NPC's.
	unsigned char special_pixels[NPIXCOLORS];   // Special colors.
	bool can_have_paperdolls = false;   // Set true if the SI paperdoll file
	//   is found when playing BG
	bool paperdolls_enabled = false;    // True if paperdolls are on.
	bool got_si_shapes = false; // Set true if the SI shapes file
	//   is found when playing BG
	using Shape_cache = std::map<std::pair<int, int>, Cached_shape>;
	Shape_cache shape_cache[static_cast<int>(SF_COUNT)];
	void read_shape_info();

public:
	friend class ShapeID;
	Shape_manager();
	~Shape_manager();
	static Shape_manager *get_instance() {
		return instance;
	}
	void load();            // Read in files.
	bool load_gumps_minimal();          // Read in files needed to display gumps.
	void reload_shapes(int shape_kind);   // Reload a shape file.
	void reload_shape_info();
	Vga_file &get_file(ShapeFile f) {
		return files[static_cast<int>(f)];
	}
	Shapes_vga_file &get_shapes() {
		return shapes;
	}
	inline Xform_palette &get_xform(int i) {
		return xforms[i];
	}
	// BG Only
	inline bool can_use_paperdolls() const {
		return can_have_paperdolls;
	}

	inline bool are_paperdolls_enabled() const {
		return paperdolls_enabled;
	}

	inline void set_paperdoll_status(bool p) {
		paperdolls_enabled = p;
	}

	inline bool have_si_shapes() const {
		return got_si_shapes;
	}

	// Paint shape in window.
	void paint_shape(int xoff, int yoff, Shape_frame *shape,
	                 bool translucent = false, unsigned char *trans = nullptr) {
		if (!shape || !shape->get_data())
			CERR("nullptr SHAPE!!!");
		else if (!shape->is_rle())
			shape->paint(xoff, yoff);
		else if (trans)
			shape->paint_rle_remapped(xoff, yoff, trans);
		else if (!translucent)
			shape->paint_rle(xoff, yoff);
		else
			shape->paint_rle_translucent(xoff, yoff, &xforms[0], xforms.size());
	}

	inline void paint_invisible(int xoff, int yoff, Shape_frame *shape) {
		if (shape) shape->paint_rle_transformed(
			    xoff, yoff, *invis_xform);
	}
	// Paint outline around a shape.
	inline void paint_outline(int xoff, int yoff, Shape_frame *shape,
	                          Pixel_colors pix) {
		if (shape) shape->paint_rle_outline(
			    xoff, yoff, special_pixels[static_cast<int>(pix)]);
	}
	unsigned char get_special_pixel(Pixel_colors pix) {
		return special_pixels[static_cast<int>(pix)];
	}

	// Paint text using "fonts.vga".
	int paint_text_box(int fontnum, const char *text, int x, int y, int w,
	                   int h, int vert_lead = 0, bool pbreak = false,
	                   bool center = false, int shading = -1, Cursor_info *cursor = nullptr);
	int paint_text(int fontnum, const char *text, int xoff, int yoff);
	int paint_text(int fontnum, const char *text, int textlen,
	               int xoff, int yoff);
	// Get text width.
	int get_text_width(int fontnum, const char *text);
	int get_text_width(int fontnum, const char *text, int textlen);
	// Get text height, baseline.
	int get_text_height(int fontnum);
	int get_text_baseline(int fontnum);
	int find_cursor(int fontnum, const char *text, int x, int y,
	                int w, int h, int cx, int cy, int vert_lead);
	Font *get_font(int fontnum);
	size_t get_xforms_cnt() const {
		return xforms.size();
	}
	Cached_shape cache_shape(int shape_kind, int shapenum, int framenum);
};

/*
 *  A shape ID contains a shape # and a frame # within the shape encoded
 *  as a 2-byte quantity.
 */
class ShapeID : public Game_singletons {
	short shapenum = -1;             // Shape #.
	signed char framenum = -1;       // Frame # within shape.
	ShapeFile shapefile = SF_SHAPES_VGA;

	Shape_manager::Cached_shape cache_shape() const;

public:
	// Read from buffer & incr. ptr.
	ShapeID(unsigned char  *&data) {
		unsigned char l = *data++;
		unsigned char h = *data++;
		shapenum = l + 256 * (h & 0x3);
		framenum = h >> 2;
	}
	// Create "end-of-list"/invalid entry.
	ShapeID() = default;

	ShapeID(const ShapeID&) = default;
	ShapeID& operator=(const ShapeID&) = default;
	ShapeID(ShapeID&&) noexcept = default;
	ShapeID& operator=(ShapeID&&) noexcept = default;
	virtual ~ShapeID() = default;
	// End-of-list or invalid?
	bool is_invalid() const {
		return shapenum == -1;
	}
	bool is_eol() const {
		return is_invalid();
	}

	inline int get_shapenum() const {
		return shapenum;
	}
	inline int get_framenum() const {
		return framenum;
	}
	inline ShapeFile get_shapefile() const {
		return shapefile;
	}
	inline Shape_frame *get_shape() const {
		return cache_shape().shape;
	}
	inline bool is_translucent() const {
		return cache_shape().has_trans;
	}
	// Set to given shape.
	void set_shape(int shnum, int frnum) {
		shapenum = shnum;
		framenum = frnum;
	}
	ShapeID(int shnum, int frnum, ShapeFile shfile = SF_SHAPES_VGA) :
		shapenum(shnum), framenum(frnum), shapefile(shfile)
	{  }

	void set_shape(int shnum) { // Set shape, but keep old frame #.
		shapenum = shnum;
	}
	void set_frame(int frnum) { // Set to new frame.
		framenum = frnum;
	}
	void set_file(ShapeFile shfile) { // Set to new flex
		shapefile = shfile;
	}

	void paint_shape(int xoff, int yoff, bool force_trans = false) {
		auto cache = cache_shape();
		sman->paint_shape(xoff, yoff, cache.shape,
		                  cache.has_trans || force_trans);
	}
	void paint_invisible(int xoff, int yoff) {
		sman->paint_invisible(xoff, yoff, get_shape());
	}
	// Paint outline around a shape.
	inline void paint_outline(int xoff, int yoff, Pixel_colors pix) {
		sman->paint_outline(xoff, yoff, get_shape(), pix);
	}
	int get_num_frames() const;
	bool is_frame_empty() const;
	const Shape_info &get_info() const {  // Get info. about shape.
		return Shape_manager::instance->shapes.get_info(shapenum);
	}
	Shape_info &get_info() { // Get info. about shape.
		return Shape_manager::instance->shapes.get_info(shapenum);
	}
	static Shape_info &get_info(int shnum) { // Get info. about shape.
		return Shape_manager::instance->shapes.get_info(shnum);
	}
};

/*
 *  An interface used in Get_click():
 */
class Paintable {
public:
	virtual void paint() = 0;
	virtual ~Paintable() = default;
};

#endif
