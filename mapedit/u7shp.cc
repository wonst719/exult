/*
 * SHP loading file filter for The GIMP version 2.x.
 *
 * (C) 2000-2001 Tristan Tarrant
 * (C) 2001-2004 Willem Jan Palenstijn
 *
 * You can find the most recent version of this file in the Exult sources,
 * available from http://exult.sf.net/
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ignore_unused_variable_warning.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif  // __GNUC__
#include <gtk/gtk.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* Declare some local functions.
 */
static void   query();
static void   run(const gchar      *name,
                  gint              nparams,
                  const GimpParam  *param,
                  gint             *nreturn_vals,
                  GimpParam       **return_vals);
static void   load_palette(gchar *filename);
static void   choose_palette();
static gint32 load_image(gchar  *filename);
static gint32 save_image(gchar  *filename,
                         gint32  image_ID,
                         gint32  drawable_ID,
                         gint32  orig_image_ID);
static GimpRunMode run_mode;

static guchar   gimp_cmap[768] = {
	0x00, 0x00, 0x00, 0xF8, 0xF0, 0xCC, 0xF4, 0xE4, 0xA4, 0xF0, 0xDC, 0x78,
	0xEC, 0xD0, 0x50, 0xEC, 0xC8, 0x28, 0xD8, 0xAC, 0x20, 0xC4, 0x94, 0x18,
	0xB0, 0x80, 0x10, 0x9C, 0x68, 0x0C, 0x88, 0x54, 0x08, 0x74, 0x44, 0x04,
	0x60, 0x30, 0x00, 0x4C, 0x24, 0x00, 0x38, 0x14, 0x00, 0xF8, 0xFC, 0xFC,
	0xFC, 0xD8, 0xD8, 0xFC, 0xB8, 0xB8, 0xFC, 0x98, 0x9C, 0xFC, 0x78, 0x80,
	0xFC, 0x58, 0x64, 0xFC, 0x38, 0x4C, 0xFC, 0x1C, 0x34, 0xDC, 0x14, 0x28,
	0xC0, 0x0C, 0x1C, 0xA4, 0x08, 0x14, 0x88, 0x04, 0x0C, 0x6C, 0x00, 0x04,
	0x50, 0x00, 0x00, 0x34, 0x00, 0x00, 0x18, 0x00, 0x00, 0xFC, 0xEC, 0xD8,
	0xFC, 0xDC, 0xB8, 0xFC, 0xCC, 0x98, 0xFC, 0xBC, 0x7C, 0xFC, 0xAC, 0x5C,
	0xFC, 0x9C, 0x3C, 0xFC, 0x8C, 0x1C, 0xFC, 0x7C, 0x00, 0xE0, 0x6C, 0x00,
	0xC0, 0x60, 0x00, 0xA4, 0x50, 0x00, 0x88, 0x44, 0x00, 0x6C, 0x34, 0x00,
	0x50, 0x24, 0x00, 0x34, 0x18, 0x00, 0x18, 0x08, 0x00, 0xFC, 0xFC, 0xD8,
	0xF4, 0xF4, 0x9C, 0xEC, 0xEC, 0x60, 0xE4, 0xE4, 0x2C, 0xDC, 0xDC, 0x00,
	0xC0, 0xC0, 0x00, 0xA4, 0xA4, 0x00, 0x88, 0x88, 0x00, 0x6C, 0x6C, 0x00,
	0x50, 0x50, 0x00, 0x34, 0x34, 0x00, 0x18, 0x18, 0x00, 0xD8, 0xFC, 0xD8,
	0xB0, 0xFC, 0xAC, 0x8C, 0xFC, 0x80, 0x6C, 0xFC, 0x54, 0x50, 0xFC, 0x28,
	0x38, 0xFC, 0x00, 0x28, 0xDC, 0x00, 0x1C, 0xC0, 0x00, 0x14, 0xA4, 0x00,
	0x0C, 0x88, 0x00, 0x04, 0x6C, 0x00, 0x00, 0x50, 0x00, 0x00, 0x34, 0x00,
	0x00, 0x18, 0x00, 0xD4, 0xD8, 0xFC, 0xB8, 0xB8, 0xFC, 0x98, 0x98, 0xFC,
	0x7C, 0x7C, 0xFC, 0x5C, 0x5C, 0xFC, 0x3C, 0x3C, 0xFC, 0x00, 0x00, 0xFC,
	0x00, 0x00, 0xE0, 0x00, 0x00, 0xC0, 0x00, 0x00, 0xA4, 0x00, 0x00, 0x88,
	0x00, 0x00, 0x6C, 0x00, 0x00, 0x50, 0x00, 0x00, 0x34, 0x00, 0x00, 0x18,
	0xE8, 0xC8, 0xE8, 0xD4, 0x98, 0xD4, 0xC4, 0x6C, 0xC4, 0xB0, 0x48, 0xB0,
	0xA0, 0x28, 0xA0, 0x8C, 0x10, 0x8C, 0x7C, 0x00, 0x7C, 0x6C, 0x00, 0x6C,
	0x60, 0x00, 0x60, 0x50, 0x00, 0x50, 0x44, 0x00, 0x44, 0x34, 0x00, 0x34,
	0x24, 0x00, 0x24, 0x18, 0x00, 0x18, 0xF4, 0xE8, 0xE4, 0xEC, 0xDC, 0xD4,
	0xE4, 0xCC, 0xC0, 0xE0, 0xC0, 0xB0, 0xD8, 0xB0, 0xA0, 0xD0, 0xA4, 0x90,
	0xC8, 0x98, 0x80, 0xC4, 0x8C, 0x74, 0xAC, 0x7C, 0x64, 0x98, 0x6C, 0x58,
	0x80, 0x5C, 0x4C, 0x6C, 0x4C, 0x3C, 0x54, 0x3C, 0x30, 0x3C, 0x2C, 0x24,
	0x28, 0x1C, 0x14, 0x10, 0x0C, 0x08, 0xEC, 0xEC, 0xEC, 0xDC, 0xDC, 0xDC,
	0xCC, 0xCC, 0xCC, 0xBC, 0xBC, 0xBC, 0xAC, 0xAC, 0xAC, 0x9C, 0x9C, 0x9C,
	0x8C, 0x8C, 0x8C, 0x7C, 0x7C, 0x7C, 0x6C, 0x6C, 0x6C, 0x60, 0x60, 0x60,
	0x50, 0x50, 0x50, 0x44, 0x44, 0x44, 0x34, 0x34, 0x34, 0x24, 0x24, 0x24,
	0x18, 0x18, 0x18, 0x08, 0x08, 0x08, 0xE8, 0xE0, 0xD4, 0xD8, 0xC8, 0xB0,
	0xC8, 0xB0, 0x90, 0xB8, 0x98, 0x70, 0xA8, 0x84, 0x58, 0x98, 0x70, 0x40,
	0x88, 0x5C, 0x2C, 0x7C, 0x4C, 0x18, 0x6C, 0x3C, 0x0C, 0x5C, 0x34, 0x0C,
	0x4C, 0x2C, 0x0C, 0x3C, 0x24, 0x0C, 0x2C, 0x1C, 0x08, 0x20, 0x14, 0x08,
	0xEC, 0xE8, 0xE4, 0xDC, 0xD4, 0xD0, 0xCC, 0xC4, 0xBC, 0xBC, 0xB0, 0xAC,
	0xAC, 0xA0, 0x98, 0x9C, 0x90, 0x88, 0x8C, 0x80, 0x78, 0x7C, 0x70, 0x68,
	0x6C, 0x60, 0x5C, 0x60, 0x54, 0x50, 0x50, 0x48, 0x44, 0x44, 0x3C, 0x38,
	0x34, 0x30, 0x2C, 0x24, 0x20, 0x20, 0x18, 0x14, 0x14, 0xE0, 0xE8, 0xD4,
	0xC8, 0xD4, 0xB4, 0xB4, 0xC0, 0x98, 0x9C, 0xAC, 0x7C, 0x88, 0x98, 0x60,
	0x70, 0x84, 0x4C, 0x5C, 0x70, 0x38, 0x4C, 0x5C, 0x28, 0x40, 0x50, 0x20,
	0x38, 0x44, 0x1C, 0x30, 0x3C, 0x18, 0x28, 0x30, 0x14, 0x20, 0x24, 0x10,
	0x18, 0x1C, 0x08, 0x0C, 0x10, 0x04, 0xEC, 0xD8, 0xCC, 0xDC, 0xB8, 0xA0,
	0xCC, 0x98, 0x7C, 0xBC, 0x80, 0x5C, 0xAC, 0x64, 0x3C, 0x9C, 0x50, 0x24,
	0x8C, 0x3C, 0x0C, 0x7C, 0x2C, 0x00, 0x6C, 0x24, 0x00, 0x60, 0x20, 0x00,
	0x50, 0x1C, 0x00, 0x44, 0x14, 0x00, 0x34, 0x10, 0x00, 0x24, 0x0C, 0x00,
	0xF0, 0xF0, 0xFC, 0xE4, 0xE4, 0xFC, 0xD8, 0xD8, 0xFC, 0xCC, 0xCC, 0xFC,
	0xC0, 0xC0, 0xFC, 0xB4, 0xB4, 0xFC, 0xA8, 0xA8, 0xFC, 0x9C, 0x9C, 0xFC,
	0x84, 0xD0, 0x00, 0x84, 0xB0, 0x00, 0x7C, 0x94, 0x00, 0x68, 0x78, 0x00,
	0x50, 0x58, 0x00, 0x3C, 0x40, 0x00, 0x2C, 0x24, 0x00, 0x1C, 0x08, 0x00,
	0x20, 0x00, 0x00, 0xEC, 0xD8, 0xC4, 0xDC, 0xC0, 0xB4, 0xCC, 0xB4, 0xA0,
	0xBC, 0x9C, 0x94, 0xAC, 0x90, 0x80, 0x9C, 0x84, 0x74, 0x8C, 0x74, 0x64,
	0x7C, 0x64, 0x58, 0x6C, 0x54, 0x4C, 0x60, 0x48, 0x44, 0x50, 0x40, 0x38,
	0x44, 0x34, 0x2C, 0x34, 0x2C, 0x24, 0x24, 0x18, 0x18, 0x18, 0x10, 0x10,
	0xFC, 0xF8, 0xFC, 0xAC, 0xD4, 0xF0, 0x70, 0xAC, 0xE4, 0x34, 0x8C, 0xD8,
	0x00, 0x6C, 0xD0, 0x30, 0x8C, 0xD8, 0x6C, 0xB0, 0xE4, 0xB0, 0xD4, 0xF0,
	0xFC, 0xFC, 0xF8, 0xFC, 0xEC, 0x40, 0xFC, 0xC0, 0x28, 0xFC, 0x8C, 0x10,
	0xFC, 0x50, 0x00, 0xC8, 0x38, 0x00, 0x98, 0x28, 0x00, 0x68, 0x18, 0x00,
	0x7C, 0xDC, 0x7C, 0x44, 0xB4, 0x44, 0x18, 0x90, 0x18, 0x00, 0x6C, 0x00,
	0xF8, 0xB8, 0xFC, 0xFC, 0x64, 0xEC, 0xFC, 0x00, 0xB4, 0xCC, 0x00, 0x70,
	0xFC, 0xFC, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFC, 0x00, 0xFC, 0x00, 0x00,
	0xFC, 0xFC, 0xFC, 0x61, 0x61, 0x61, 0xC0, 0xC0, 0xC0, 0xFC, 0x00, 0xF1
};

GimpPlugInInfo PLUG_IN_INFO = {
	nullptr,  /* init_proc  */
	nullptr,  /* quit_proc  */
	query, /* query_proc */
	run,   /* run_proc   */
};

struct u7frame {
	guchar *pixels;
	size_t datalen;
	gint16 leftX;
	gint16 leftY;
	gint16 rightX;
	gint16 rightY;
	gint16 width;
	gint16 height;
};

struct u7shape {
	u7frame *frames;
	size_t num_frames;
};


MAIN()	// NOLINT

static void query() {
	static GimpParamDef load_args[] = {
		{ GIMP_PDB_INT32,  g_strdup("run_mode"),     g_strdup("Interactive, non-interactive") },
		{ GIMP_PDB_STRING, g_strdup("filename"),     g_strdup("The name of the file to load") },
		{ GIMP_PDB_STRING, g_strdup("raw_filename"), g_strdup("The name entered") }
	};
	static GimpParamDef load_return_vals[] = {
		{ GIMP_PDB_IMAGE, g_strdup("image"), g_strdup("Output image") }
	};
	static gint nload_args = sizeof(load_args) / sizeof(load_args[0]);
	static gint nload_return_vals = (sizeof(load_return_vals) /
	                                 sizeof(load_return_vals[0]));

	static GimpParamDef save_args[] = {
		{ GIMP_PDB_INT32,    g_strdup("run_mode"),     g_strdup("Interactive, non-interactive") },
		{ GIMP_PDB_IMAGE,    g_strdup("image"),        g_strdup("Image to save") },
		{ GIMP_PDB_DRAWABLE, g_strdup("drawable"),     g_strdup("Drawable to save") },
		{ GIMP_PDB_STRING,   g_strdup("filename"),     g_strdup("The name of the file to save") },
		{ GIMP_PDB_STRING,   g_strdup("raw_filename"), g_strdup("The name entered") }
	};
	static gint nsave_args = sizeof(save_args) / sizeof(save_args[0]);

	gimp_install_procedure("file_shp_load",
	                       "loads files in Ultima 7 SHP format",
	                       "FIXME: write help for shp_load",
	                       "Tristan Tarrant",
	                       "Tristan Tarrant",
	                       "2000",
	                       "<Load>/SHP",
	                       nullptr,
	                       GIMP_PLUGIN,
	                       nload_args, nload_return_vals,
	                       load_args, load_return_vals);

	gimp_register_magic_load_handler("file_shp_load",
	                                 "shp",
	                                 "",
	                                 "");

	gimp_install_procedure("file_shp_save",
	                       "Save files in Ultima 7 SHP format",
	                       "FIXME: write help for shp_save",
	                       "Tristan Tarrant",
	                       "Tristan Tarrant",
	                       "2000",
	                       "<Save>/SHP",
	                       "INDEXEDA",
	                       GIMP_PLUGIN,
	                       nsave_args, 0,
	                       save_args, nullptr);

	gimp_register_save_handler("file_shp_save",
	                           "shp",
	                           "");
}

static void run(
    const gchar      *name,
    gint              nparams,
    const GimpParam  *param,
    gint             *nreturn_vals,
    GimpParam       **return_vals
) {
	ignore_unused_variable_warning(nparams);
	static GimpParam  values[2];

	gegl_init(nullptr, nullptr);

	run_mode = static_cast<GimpRunMode>(param[0].data.d_int32);

	*nreturn_vals = 1;
	*return_vals  = values;
	values[0].type          = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	if (strcmp(name, "file_shp_load") == 0) {
		gimp_ui_init("u7shp", FALSE);
		if (run_mode != GIMP_RUN_NONINTERACTIVE) {
			choose_palette();
		}
		gint32 image_ID = load_image(param[1].data.d_string);

		if (image_ID != -1) {
			*nreturn_vals = 2;
			values[1].type         = GIMP_PDB_IMAGE;
			values[1].data.d_image = image_ID;
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
		}
	} else if (strcmp(name, "file_shp_save") == 0) {
		gint32 orig_image_ID = param[1].data.d_int32;
		gint32 image_ID    = orig_image_ID;
		gint32 drawable_ID = param[2].data.d_int32;
		save_image(param[3].data.d_string,
		           image_ID,
		           drawable_ID,
		           orig_image_ID);
	} else {
		status = GIMP_PDB_CALLING_ERROR;
	}
	values[0].data.d_status = status;
}

unsigned int read1(FILE *f) {
	unsigned char b0 = fgetc(f);
	return b0;
}

unsigned int read2(FILE *f) {
	unsigned char b0 = fgetc(f);
	unsigned char b1 = fgetc(f);
	return b0 + (b1 << 8);
}

/* Flauschepelz */
signed int read2signed(FILE *f) {
	unsigned char b0 = fgetc(f);
	unsigned char b1 = fgetc(f);
	signed int i0 = b0 + (b1 << 8);
	if (i0 >= 32768) {
		i0 = i0 - 65536;
	}
	return i0;
}

unsigned int read4(FILE *f) {
	unsigned char b0 = fgetc(f);
	unsigned char b1 = fgetc(f);
	unsigned char b2 = fgetc(f);
	unsigned char b3 = fgetc(f);
	return b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
}

void write1(FILE *f, unsigned char b) {
	fputc(b, f);
}

void write2(FILE *f, unsigned int b) {
	fputc(b & 0xFF, f);
	fputc((b >> 8) & 0xFF, f);
}

void write4(FILE *f, unsigned int b) {
	fputc(b & 0xFF, f);
	fputc((b >> 8) & 0xFF, f);
	fputc((b >> 16) & 0xFF, f);
	fputc((b >> 24) & 0xFF, f);
}

unsigned char *out1(unsigned char *p, unsigned char b) {
	*p++ = b;
	return p;
}

unsigned char *out2(unsigned char *p, unsigned int b) {
	*p++ = b & 0xFF;
	*p++ = (b >> 8) & 0xFF;
	return p;
}

unsigned char *out4(unsigned char *p, unsigned int b) {
	*p++ = b & 0xFF;
	*p++ = (b >> 8) & 0xFF;
	*p++ = (b >> 16) & 0xFF;
	*p++ = (b >> 24) & 0xFF;
	return p;
}

static void load_palette(gchar *filename) {
	FILE *fp = fopen(filename, "rb");
	g_free(filename);
	if (!fp) {
		return;
	}
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (len == 768) {
		for (unsigned i = 0; i < 256; i++) {
			gimp_cmap[i * 3] = read1(fp) << 2;
			gimp_cmap[i * 3 + 1] = read1(fp) << 2;
			gimp_cmap[i * 3 + 2] = read1(fp) << 2;
		}
	} else if (len == 1536) {
		for (unsigned i = 0; i < 256; i++) {
			gimp_cmap[i * 3] = read1(fp) << 2;
			read1(fp);
			gimp_cmap[i * 3 + 1] = read1(fp) << 2;
			read1(fp);
			gimp_cmap[i * 3 + 2] = read1(fp) << 2;
			read1(fp);
		}
	}
	fclose(fp);
}

static void file_sel_delete(GtkWidget *widget, GtkWidget **file_sel) {
	ignore_unused_variable_warning(widget);
	gtk_widget_destroy(*file_sel);
	*file_sel = nullptr;
}

static void file_selected(GtkWidget *widget, gboolean *selected) {
	ignore_unused_variable_warning(widget);
	*selected = TRUE;
}

gchar *file_select(const gchar *title) {
	GtkWidget *file_sel = gtk_file_selection_new(title);
	gtk_window_set_modal(GTK_WINDOW(file_sel), TRUE);

	gtk_signal_connect(GTK_OBJECT(file_sel), "destroy", GTK_SIGNAL_FUNC(file_sel_delete), &file_sel);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_sel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(file_sel_delete), &file_sel);

	gboolean selected = FALSE;
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_sel)->ok_button), "clicked", GTK_SIGNAL_FUNC(file_selected), &selected);

	gtk_widget_show(file_sel);

	while (!selected && file_sel) {
		gtk_main_iteration();
	}

	/* canceled or window was closed */
	if (! selected) {
		return nullptr;
	}

	/* ok */
	gchar *filename = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_sel)));
	gtk_widget_destroy(file_sel);
	return filename;
}

static void choose_palette() {
	load_palette(file_select("Choose palette"));
}

static gint32 load_image(gchar *filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		g_message("SHP: can't open \"%s\"\n", filename);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	const size_t file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	const size_t shape_size = read4(fp);

	u7shape shape;
	gint16 max_leftX = -1;
	gint16 max_rightX = -1;
	gint16 max_leftY = -1;
	gint16 max_rightY = -1;
	GimpImageType image_type;
	if (file_size != shape_size) {
		// 8x8 tile
		image_type = GIMP_INDEXED_IMAGE;
		shape.num_frames = file_size / 64;
		fseek(fp, 0, SEEK_SET);
#ifdef DEBUG
		printf("num_frames = %d\n", shape.num_frames);
#endif
		shape.frames = g_new(u7frame, shape.num_frames);
		max_leftX = 0;
		max_leftY = 0;
		max_rightX = 7;
		max_rightY = 7;
		for (size_t i = 0; i < shape.num_frames; i++) {
			u7frame &frame = shape.frames[i];
			frame.width = 8;
			frame.height = 8;
			frame.leftX = 0;
			frame.leftY = 0;
			frame.pixels = g_new(guchar, 64);
			if (fread(frame.pixels, 1, 64, fp) != 64) {
				// Silently fail
				memset(frame.pixels, 0, 64);
			}
		}
	} else {
		image_type = GIMP_INDEXEDA_IMAGE;
		const gint32 hdr_size = read4(fp);
		shape.num_frames = (hdr_size - 4) / 4;
#ifdef DEBUG
		printf("num_frames = %d\n", shape.num_frames);
#endif
		shape.frames = g_new(u7frame, shape.num_frames);

		for (size_t i = 0; i < shape.num_frames; i++) {
			u7frame &frame = shape.frames[i];
			// Go to where frame offset is stored
			fseek(fp, (i + 1) * 4, SEEK_SET);
			const gint32 frame_offset = read4(fp);
			fseek(fp, frame_offset, SEEK_SET);
			frame.rightX = read2(fp);
			frame.leftX = read2(fp);
			frame.leftY = read2(fp);
			frame.rightY = read2(fp);

			if (frame.leftX > max_leftX)
				max_leftX = frame.leftX;
			if (frame.rightX > max_rightX)
				max_rightX = frame.rightX;
			if (frame.leftY > max_leftY)
				max_leftY = frame.leftY;
			if (frame.rightY > max_rightY)
				max_rightY = frame.rightY;

			frame.width    = frame.leftX + frame.rightX + 1;
			frame.height   = frame.leftY + frame.rightY + 1;
			frame.pixels   = g_new0(guchar, frame.width * frame.height * 2);
			guchar *pixptr = frame.pixels;
			for (gint16 slice = read2(fp); slice != 0; slice = read2(fp)) {
				const gint16 slice_type   = slice & 0x1;
				gint16       slice_length = slice >> 1;

				const signed int offsetX = read2signed(fp);
				const signed int offsetY = read2signed(fp);
				const signed int offset  = (frame.leftY + offsetY) * frame.width * 2 +
				           (frame.leftX + offsetX) * 2;

				pixptr = frame.pixels + offset;

				if (pixptr < frame.pixels)
					pixptr = frame.pixels;
				if (slice_type) {   // Compressed
					while (slice_length > 0) {
						const guchar block = read1(fp);
						const gint16 block_type = block & 0x1;
						const gint16 block_length = block >> 1;
						if (block_type) {
							const guchar pix = read1(fp);
							for (gint16 j = 0; j < block_length; j++) {
								*pixptr++ = pix;
								*pixptr++ = 255;
							}
						} else {
							for (gint16 j = 0; j < block_length; j++) {
								const guchar pix = read1(fp);
								*pixptr++ = pix;
								*pixptr++ = 255;
							}
						}
						slice_length -= block_length;
					}
				} else {        // Uncompressed
					// Just read the pixels
					for (gint16 j = 0; j < slice_length; j++) {
						const guchar pix = read1(fp);
						*pixptr++ = pix;
						*pixptr++ = 255;
					}
				}
			}
		}
	}
	const gint32 image_ID = gimp_image_new(max_leftX + max_rightX + 1,
	                          max_leftY + max_rightY + 1, GIMP_INDEXED);
	gimp_image_set_filename(image_ID, filename);
	gimp_image_set_colormap(image_ID, gimp_cmap, 256);
	for (size_t i = 0; i < shape.num_frames; i++) {
		gchar *framename = g_strdup_printf("Frame %zu", i);
		u7frame &frame = shape.frames[i];
		const gint32 layer_ID = gimp_layer_new(image_ID, framename,
		                          frame.width, frame.height,
		                          image_type, 100, GIMP_NORMAL_MODE);
		g_free(framename);
		gimp_image_insert_layer(image_ID, layer_ID, -1, 0);
		gimp_item_transform_translate(layer_ID, max_leftX - frame.leftX,
		                              max_leftY - frame.leftY);

		GeglBuffer *drawable = gimp_drawable_get_buffer(layer_ID);
		const GeglRectangle rect{0, 0,
		                         gegl_buffer_get_width(drawable),
		                         gegl_buffer_get_height(drawable)};
		gegl_buffer_set(drawable, &rect,
		                0, nullptr, frame.pixels, GEGL_AUTO_ROWSTRIDE);
		g_object_unref(drawable);
	}

	fclose(fp);

	for (size_t i = 0; i < shape.num_frames; i++) {
		g_free(shape.frames[i].pixels);
	}

	g_free(shape.frames);

	gimp_image_add_hguide(image_ID, max_leftY);
#ifdef DEBUG
	printf("Added hguide=%d\n", max_leftY);
#endif
	gimp_image_add_vguide(image_ID, max_leftX);
#ifdef DEBUG
	printf("Added vguide=%d\n", max_leftX);
#endif

	return image_ID;
}

static int find_runs(short *runs, const unsigned char *pixptr, int x, int w) {
	int runcnt = 0;
	while (x < w && pixptr[1] != 0) {   // Stop at first transparent pixel.
		int run = 0;        // Look for repeat.
		while (x < w - 1 && pixptr[0] == pixptr[2] && pixptr[3] != 0) {
#ifdef DEBUG
			if (pixptr[3] == 0)
				printf("Warning: found pixel pair, but second is transparent\n");
#endif
			x++;
			pixptr += 2;
			run++;
		}
		if (run) {      // Repeated?  Count 1st, shift, flag.
			run = ((run + 1) << 1) | 1;
			x++;        // Also pass the last one.
			pixptr += 2;
		} else {
			do {            /* Pass non-repeated run of */
				x++;
				pixptr += 2;
				run += 2;   // So we don't have to shift.
			} while (x < w && pixptr[1] != 0 &&
			         (x == w - 1 || pixptr[0] != pixptr[2]));
		}
		// Store run length.
		runs[runcnt++] = run;
	}
	runs[runcnt] = 0;       /* 0-delimit list. */
	return x;
}

static int skip_transparent(unsigned char **pixptr, int x, int w) {
	unsigned char *pixel = *pixptr;
	while (x < w && pixel[1] == 0) {    /* Pixel is transparent if alpha is 0 */
		x++;
		pixel += 2;
	}
	*pixptr = pixel;
	return x;
}

static gint32 save_image(gchar  *filename,
                         gint32  image_ID,
                         gint32  drawable_ID,
                         gint32  orig_image_ID) {
	ignore_unused_variable_warning(drawable_ID, orig_image_ID);
	if (run_mode != GIMP_RUN_NONINTERACTIVE) {
		gchar *name_buf = g_strdup_printf("Saving %s:", filename);
		gimp_progress_init(name_buf);
		g_free(name_buf);
	}

	// Find the guides...
	gint32 guide_ID = gimp_image_find_next_guide(image_ID, 0);
	int hotx = -1;
	int hoty = -1;
	while (guide_ID > 0) {
		int orientation;
#ifdef DEBUG
		printf("Found guide %d:", guide_ID);
#endif
		orientation = gimp_image_get_guide_orientation(image_ID, guide_ID);
		switch (orientation) {
		case GIMP_ORIENTATION_HORIZONTAL:
			if (hoty < 0) {
				hoty = gimp_image_get_guide_position(image_ID, guide_ID);
#ifdef DEBUG
				printf(", horizontal=%d\n", hoty);
#endif
			}
			break;
		case GIMP_ORIENTATION_VERTICAL:
			if (hotx < 0) {
				hotx = gimp_image_get_guide_position(image_ID, guide_ID);
#ifdef DEBUG
				printf(", vertical=%d\n", hotx);
#endif
			}
			break;
		default:
			break;
		}
		guide_ID = gimp_image_find_next_guide(image_ID, guide_ID);
	}

	// get a list of layers for this image_ID
	int nlayers;
	gint32 *layers = gimp_image_get_layers(image_ID, &nlayers);

	if (nlayers > 0 && !gimp_drawable_is_indexed(layers[0])) {
		g_message("SHP: You can only save indexed images!");
		return -1;
	}

	u7shape shape;
	shape.num_frames = nlayers;
	shape.frames = g_new(u7frame, shape.num_frames);

	for (int k = 0; k < nlayers; k++) {
		u7frame& frame = shape.frames[k];
		GeglBuffer *drawable = gimp_drawable_get_buffer(layers[k]);
		gint offsetX;
		gint offsetY;
		gimp_drawable_offsets(layers[k], &offsetX, &offsetY);
		frame.width = gegl_buffer_get_width(drawable);
		frame.height = gegl_buffer_get_height(drawable);
		frame.leftX = hotx - offsetX;
		frame.leftY = hoty - offsetY;
		frame.rightX = frame.width - frame.leftX - 1;
		frame.rightY = frame.height - frame.leftY - 1;
		const Babl *format = gegl_buffer_get_format(drawable);
		auto   *pix    = g_new(guchar, frame.width * frame.height * babl_format_get_bytes_per_pixel(format));
		guchar *pixptr = pix;
		auto   *out    = g_new(guchar, frame.width * frame.height * 8);
		guchar *outptr = out;
		const GeglRectangle rect{0, 0,
		                         gegl_buffer_get_width(drawable),
		                         gegl_buffer_get_height(drawable)};
		gegl_buffer_get(drawable, &rect,
		                1.0, format, pixptr, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
		g_object_unref(drawable);

		int newx = 0;
		for (int y = 0; y < frame.height; y++) {
			for (int x = 0; (x = skip_transparent(&pixptr, x, frame.width)) < frame.width; x = newx) {
				short runs[100];
				newx = find_runs(runs, pixptr, x, frame.width);
				if (!runs[1] && !(runs[0] & 1)) {
					int len = runs[0] >> 1;
					outptr = out2(outptr, runs[0]);
					outptr = out2(outptr, x - frame.leftX);
					outptr = out2(outptr, y - frame.leftY);
					for (int i = 0; i < len; i++) {
						*outptr++ = *pixptr;
						pixptr += 2;
					}
				} else {
					outptr = out2(outptr, ((newx - x) << 1) | 1);
					outptr = out2(outptr, x - frame.leftX);
					outptr = out2(outptr, y - frame.leftY);
					for (int i = 0; runs[i]; i++) {
						int len = runs[i] >> 1;
						if (runs[i] & 1) {
							while (len) {
								int c = len > 127 ? 127 : len;
								*outptr++ = (c << 1) | 1;
								*outptr++ = *pixptr;
								pixptr += (c * 2);
								len -= c;
							}
						} else while (len > 0) {
							int c;
							c = len > 127 ? 127 : len;
							*outptr++ = c << 1;
							for (int j = 0; j < c; j++) {
								*outptr++ = *pixptr;
								pixptr += 2;
							}
							len -= c;
						}
					}
				}
			}
		}
		outptr = out2(outptr, 0);           // End with 0 length.
		frame.datalen = outptr - out;
		frame.pixels = g_new(guchar, frame.datalen);
		memcpy(frame.pixels, out, frame.datalen);
		g_free(out);
		g_free(pix);
	}

	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		g_message("SHP: can't create \"%s\"\n", filename);
		g_free(shape.frames);
		return -1;
	}

	write4(fp, 0); // Fill in later
	for (size_t i = 0; i < shape.num_frames; i++)
		write4(fp, 0);  // Fill in later
	for (size_t i = 0; i < shape.num_frames; i++) {
		u7frame &frame = shape.frames[shape.num_frames - i - 1];
		size_t pos = ftell(fp);    // Get the frame offset
		fseek(fp, (i + 1) * 4, SEEK_SET);
		write4(fp, pos);    // Write it in the right place
		fseek(fp, pos, SEEK_SET);
		write2(fp, frame.rightX);
		write2(fp, frame.leftX);
		write2(fp, frame.leftY);
		write2(fp, frame.rightY);
		fwrite(frame.pixels, 1, frame.datalen, fp);
	}

	size_t pos = ftell(fp);    // Get the file size
	fseek(fp, 0, SEEK_SET);
	write4(fp, pos);    // Write it in the right place

	fclose(fp);

	for (size_t i = 0; i < shape.num_frames; i++) {
		g_free(shape.frames[i].pixels);
	}
	g_free(shape.frames);

	return 0;
}
