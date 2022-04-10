AM_CPPFLAGS = -I$(top_srcdir)/headers -I$(top_srcdir) -I$(top_srcdir)/imagewin -I$(top_srcdir)/files \
		-I$(top_srcdir)/objs -I$(top_srcdir)/shapes\
	 $(SDL_CFLAGS) $(INCDIRS) $(WINDOWING_SYSTEM) $(DEBUG_FLAGS) $(CPPFLAGS)

noinst_LTLIBRARIES = libflic.la

libflic_la_SOURCES =	\
	playfli.cc	\
	playfli.h

CLEANFILES = *~
