AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/headers -I$(top_srcdir)/objs $(SDL_CFLAGS) $(INCDIRS) \
		$(WINDOWING_SYSTEM) $(DEBUG_FLAGS) $(CPPFLAGS)

noinst_LTLIBRARIES = libpathfinder.la

libpathfinder_la_SOURCES =	\
	Astar.cc		\
	Astar.h			\
	PathFinder.cc		\
	PathFinder.h		\
	Zombie.cc		\
	Zombie.h		\
	path.cc

CLEANFILES = *~
