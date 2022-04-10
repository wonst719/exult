AM_CPPFLAGS = -I$(top_srcdir)/headers -I$(top_srcdir)/usecode/ucxt/include -I$(top_srcdir)/conf \
	-I$(top_srcdir)/usecode -I$(top_srcdir) $(INCDIRS) $(DEBUG_FLAGS) $(CPPFLAGS) \
	-DEXULT_DATADIR=\"$(EXULT_DATADIR)\" $(WARNINGS)

ucxt_SOURCES = \
	ops.cc \
	ucdata.cc \
	ucxt.cc \
	ucfunc.cc

ucxt_LDADD = \
	../../../conf/libconf.la \
	../../../gamemgr/libgamemgr.la \
	../../../files/libu7file.la \
	../../../files/zip/libminizip.la \
	../../../usecode/libusecode.la \
	$(ZLIB_LIBS) \
	$(SYSLIBS)

if BUILD_TOOLS
bin_PROGRAMS = ucxt
endif
