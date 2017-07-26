if HAVE_WEBKITGTKWEBEXT
webextension_LTLIBRARIES = libcfmuwebextension.la

webextensiondir = $(pkglibexecdir)/web-extensions

libcfmuwebextension_la_SOURCES = \
	cfmu-web-extension.c \
	cfmu-web-extension.h \
	cfmu-web-extension-main.c \
	cfmu-web-extension-names.h

libcfmuwebextension_la_LDFLAGS = \
	-module -avoid-version -no-undefined

libcfmuwebextension_la_LIBADD = \
	@GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ @WEBKITGTKWEBEXT_LIBS@

libcfmuwebextension_la_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTKWEBEXT_CFLAGS@

endif

noinst_LTLIBRARIES = libcfmuvalidationwidget.la

BUILT_SOURCES = \
	cfmu-embed-type-builtins.c \
	cfmu-embed-type-builtins.h

libcfmuvalidationwidget_la_SOURCES = \
	cfmuvalidatewidget.cc \
	cfmuvalidatewidget.hh

EXTRA_DIST = \
	cfmu-web-extension-proxy.c \
	cfmu-web-extension-proxy.h \
	cfmu-web-extension.c \
	cfmu-web-extension.h \
	cfmu-web-extension-main.c \
	cfmu-web-extension-names.h \
	interface.xml

if HAVE_WEBKITGTK4

libcfmuvalidationwidget_la_SOURCES += \
	cfmu-web-extension-proxy.c \
	cfmu-web-extension-proxy.h

libcfmuvalidationwidget_la_LIBADD = \
	@GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ @WEBKITGTK4_LIBS@

libcfmuvalidationwidget_la_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTK4_CFLAGS@

else

if HAVE_WEBKITGTK

libcfmuvalidationwidget_la_LIBADD = \
	@GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ @WEBKITGTK_LIBS@

libcfmuvalidationwidget_la_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTK_CFLAGS@

endif
endif

nodist_libcfmuvalidationwidget_la_SOURCES = \
	$(BUILT_SOURCES)

CLEANFILES = $(stamp_files) $(BUILT_SOURCES)

TYPES_H_FILES = \
	cfmu-web-extension-proxy.h

stamp_files = \
	stamp-cfmu-embed-type-builtins.c	\
	stamp-cfmu-embed-type-builtins.h

cfmu-embed-type-builtins.c: stamp-cfmu-embed-type-builtins.c Makefile
	@true
stamp-cfmu-embed-type-builtins.c: Makefile $(TYPES_H_FILES)
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include <config.h>\n\n" \
		--fhead "#include \"cfmu-embed-type-builtins.h\"\n\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--fprod "\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n" \
		--vhead "  static GType type = 0;\n\n" \
		--vhead "  if (G_UNLIKELY (type == 0))\n  {\n" \
		--vhead "    static const G@Type@Value _@enum_name@_values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n\n" \
		--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n  }\n\n" \
		--vtail "  return type;\n}\n\n" \
		$(filter-out $<,$^) > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%) ) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

cfmu-embed-type-builtins.h: stamp-cfmu-embed-type-builtins.h Makefile
	@true
stamp-cfmu-embed-type-builtins.h: Makefile $(TYPES_H_FILES)
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef CFMU_EMBED_TYPE_BUILTINS_H\n" \
		--fhead "#define CFMU_EMBED_TYPE_BUILTINS_H 1\n\n" \
		--fhead "#include <glib-object.h>\n\n" \
		--fhead "G_BEGIN_DECLS\n\n" \
		--ftail "G_END_DECLS\n\n" \
		--ftail "#endif /* CFMU_EMBED_TYPE_BUILTINS_H */\n" \
		--fprod "\n/* --- @filename@ --- */" \
		--eprod "#define CFMU_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
		--eprod "GType @enum_name@_get_type (void);\n" \
		$(filter-out $<,$^) > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%) ) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)

AM_CFLAGS = @MY_CFLAGS@
AM_CXXFLAGS = @MY_CXXFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include -I$(top_srcdir)/ead \
        @GTKMM_CFLAGS@ @GLIBMM_CFLAGS@ @GIOMM_CFLAGS@
