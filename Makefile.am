AUTOMAKE_OPTIONS = subdir-objects

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-more-warnings=yes

libexec_PROGRAMS =

noinst_LTLIBRARIES =

EXTRA_DIST =

CLEANFILES =

check_programs =

noinst_PROGRAMS =

SUBDIRS = \
	. \
	po

plugindir = $(libdir)/NetworkManager
plugin_LTLIBRARIES =

dbusservicedir = $(datadir)/dbus-1/system.d
dbusservice_DATA = nm-anyconnect-service.conf

nmvpnservicedir = $(NM_VPN_SERVICE_DIR)
nmvpnservice_DATA = nm-anyconnect-service.name

uidir = $(datadir)/gnome-vpn-properties/anyconnect
ui_DATA =

###############################################################################

shared_sources = \
	shared/nm-utils/nm-shared-utils.c

###############################################################################

src_cppflags = \
	-DBINDIR=\"$(bindir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	-DLOCALSTATEDIR=\""$(localstatedir)"\" \
	-DDATADIR=\"$(datadir)\" \
	-DNM_ANYCONNECT_LOCALEDIR=\"$(datadir)/locale\" \
	-DG_LOG_DOMAIN=\"nm-anyconnect\" \
	-DNETWORKMANAGER_COMPILATION=NM_NETWORKMANAGER_COMPILATION_DEFAULT \
	-I$(srcdir)/shared \
	$(GLIB_CFLAGS) \
	$(LIBNM_CFLAGS)

noinst_LTLIBRARIES += src/libnm-utils.la

src_libnm_utils_la_SOURCES = $(shared_sources)
src_libnm_utils_la_CPPFLAGS = $(src_cppflags)
src_libnm_utils_la_LIBADD = \
	$(GLIB_LIBS) \
	$(LIBNM_LIBS)

libexec_PROGRAMS += src/nm-anyconnect-service

src_nm_anyconnect_service_CPPFLAGS = $(src_cppflags)
src_nm_anyconnect_service_LDFLAGS = \
	-Wl,--version-script="$(srcdir)/linker-script-binary.ver"
src_nm_anyconnect_service_LDADD = \
	src/libnm-utils.la \
	$(GLIB_LIBS) \
	$(LIBNM_LIBS)
EXTRA_src_nm_anyconnect_service_DEPENDENCIES = \
	linker-script-binary.ver

###############################################################################

properties/resources.h: properties/gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ --sourcedir=$(srcdir)/properties --generate-header --internal

properties/resources.c: properties/gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/properties --generate-dependencies $(srcdir)/properties/gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ --sourcedir=$(srcdir)/properties --generate-source --internal

CLEANFILES += \
	properties/resources.c \
	properties/resources.h

EXTRA_DIST += \
	properties/gresource.xml

plugin_sources = \
	properties/nm-anyconnect-editor-plugin.c \
	properties/nm-anyconnect-editor-plugin.h

editor_sources = \
	properties/resources.c \
	properties/resources.h \
	properties/nm-anyconnect-editor.c \
	properties/nm-anyconnect-editor.h

properties_cppflags = \
	-DICONDIR=\""$(datadir)/pixmaps"\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-I$(srcdir)/shared \
	$(GLIB_CFLAGS)

###############################################################################

noinst_LTLIBRARIES += properties/libnm-vpn-plugin-anyconnect-utils.la

properties_libnm_vpn_plugin_anyconnect_utils_la_SOURCES = \
	$(shared_sources)

properties_libnm_vpn_plugin_anyconnect_utils_la_CPPFLAGS = \
	-DNETWORKMANAGER_COMPILATION=NM_NETWORKMANAGER_COMPILATION_LIB_BASE \
	-DNM_PLUGIN_DIR=\"$(NM_PLUGIN_DIR)\" \
	$(properties_cppflags) \
	$(LIBNM_CFLAGS)

properties_libnm_vpn_plugin_anyconnect_utils_la_LIBADD = \
	$(LIBNM_LIBS) \
	$(DL_LIBS)

noinst_LTLIBRARIES += properties/libnm-vpn-plugin-anyconnect-core.la

properties_libnm_vpn_plugin_anyconnect_core_la_SOURCES = \
	shared/nm-utils/nm-vpn-plugin-utils.c \
	shared/nm-utils/nm-vpn-plugin-utils.h \
	shared/nm-utils/nm-vpn-plugin-macros.h \
	$(plugin_sources)

properties_libnm_vpn_plugin_anyconnect_core_la_CPPFLAGS = \
	-DNETWORKMANAGER_COMPILATION=NM_NETWORKMANAGER_COMPILATION_LIB_BASE \
	-DNM_PLUGIN_DIR=\"$(NM_PLUGIN_DIR)\" \
	$(properties_cppflags) \
	$(LIBNM_CFLAGS)

properties_libnm_vpn_plugin_anyconnect_core_la_LIBADD = \
	properties/libnm-vpn-plugin-anyconnect-utils.la \
	$(LIBNM_LIBS) \
	$(DL_LIBS)

plugin_LTLIBRARIES += properties/libnm-vpn-plugin-anyconnect.la

properties_libnm_vpn_plugin_anyconnect_la_SOURCES =
properties_libnm_vpn_plugin_anyconnect_la_LIBADD = \
	properties/libnm-vpn-plugin-anyconnect-core.la
properties_libnm_vpn_plugin_anyconnect_la_LDFLAGS = \
	-avoid-version \
	-Wl,--version-script=$(srcdir)/properties/libnm-vpn-plugin-anyconnect.ver

###############################################################################

if WITH_GNOME
plugin_LTLIBRARIES += properties/libnm-vpn-plugin-anyconnect-editor.la
endif

properties_libnm_vpn_plugin_anyconnect_editor_la_SOURCES = \
	$(editor_sources)

properties_libnm_vpn_plugin_anyconnect_editor_la_CPPFLAGS = \
	-DNETWORKMANAGER_COMPILATION=NM_NETWORKMANAGER_COMPILATION_LIB_EDITOR \
	$(properties_cppflags) \
	$(GTK_CFLAGS) \
	$(LIBNM_CFLAGS) \
	$(LIBNMA_CFLAGS)

properties_libnm_vpn_plugin_anyconnect_editor_la_LIBADD = \
	properties/libnm-vpn-plugin-anyconnect-utils.la \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(LIBNM_LIBS) \
	$(LIBNMA_LIBS)

properties_libnm_vpn_plugin_anyconnect_editor_la_LDFLAGS = \
	-avoid-version \
	-Wl,--version-script=$(srcdir)/properties/libnm-vpn-plugin-anyconnect-editor.ver

###############################################################################

if WITH_LIBNM_GLIB
noinst_LTLIBRARIES += properties/libnm-anyconnect-properties-core.la
endif

properties_libnm_anyconnect_properties_core_la_SOURCES = \
	$(shared_sources) \
	$(plugin_sources) \
	$(editor_sources)

properties_libnm_anyconnect_properties_core_la_CPPFLAGS = \
	-DNETWORKMANAGER_COMPILATION='NM_NETWORKMANAGER_COMPILATION_LIB|NM_NETWORKMANAGER_COMPILATION_WITH_LIBNM_UTIL' \
	$(properties_cppflags) \
	$(GTK_CFLAGS) \
	$(LIBNM_GLIB_CFLAGS) \
	$(LIBNM_GTK_CFLAGS)

properties_libnm_anyconnect_properties_core_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(LIBNM_GLIB_LIBS) \
	$(LIBNM_GTK_LIBS)


if WITH_LIBNM_GLIB
plugin_LTLIBRARIES += properties/libnm-anyconnect-properties.la
endif

properties_libnm_anyconnect_properties_la_SOURCES =
properties_libnm_anyconnect_properties_la_LIBADD = \
	properties/libnm-anyconnect-properties-core.la
properties_libnm_anyconnect_properties_la_LDFLAGS = \
	-avoid-version \
	-Wl,--version-script=$(srcdir)/properties/libnm-anyconnect-properties.ver

EXTRA_DIST += \
	properties/libnm-vpn-plugin-anyconnect.ver \
	properties/libnm-vpn-plugin-anyconnect-editor.ver \
	properties/libnm-anyconnect-properties.ver \
	properties/nm-anyconnect-dialog.ui

###############################################################################

if WITH_LIBNM_GLIB
# Install a file with full path to plugins for an old gnome-shell
# https://bugzilla.gnome.org/show_bug.cgi?id=693590
install-data-hook:
	mkdir -p $(DESTDIR)$(sysconfdir)/NetworkManager/VPN
	sed -e "1s|^|# This file is obsoleted by a file in $(NM_VPN_SERVICE_DIR)\n\n|" \
	    -e 's|[@]LIBEXECDIR[@]|$(libexecdir)|g' \
	    -e 's|[@]PLUGINDIR[@]|@NM_PLUGIN_DIR@|g' \
	    <$(srcdir)/nm-anyconnect-service.name.in \
	    >$(DESTDIR)$(sysconfdir)/NetworkManager/VPN/nm-anyconnect-service.name

uninstall-hook:
	 rm -f $(DESTDIR)$(sysconfdir)/NetworkManager/VPN/nm-anyconnect-service.name
endif

appdatadir = $(datadir)/metainfo
appdata_files = $(appdata_in_files:.xml.in=.xml)
if WITH_GNOME
appdata_DATA = $(appdata_files)
endif
appdata_in_files = appdata/network-manager-anyconnect.metainfo.xml.in
@INTLTOOL_XML_RULE@

nm-anyconnect-service.name: $(srcdir)/nm-anyconnect-service.name.in
	$(AM_V_GEN) sed -e 's|[@]LIBEXECDIR[@]|$(libexecdir)|g' \
	    -e 's|[@]PLUGINDIR[@]/|@NM_PLUGIN_DIR_NAME_FILE@|g' \
	    $^ >$@

EXTRA_DIST += \
	linker-script-binary.ver \
	nm-anyconnect-service.name.in \
	$(dbusservice_DATA) \
	$(appdata_in_files) \
	$(appdata_files) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	\
	shared/README \
	shared/nm-utils/gsystem-local-alloc.h \
	shared/nm-utils/nm-glib.h \
	shared/nm-utils/nm-macros-internal.h \
	shared/nm-utils/nm-shared-utils.c \
	shared/nm-utils/nm-shared-utils.h \
	shared/nm-utils/nm-test-utils.h \
	shared/nm-default.h \
	shared/nm-service-defines.h \
	\
	rpm/NetworkManager-anyconnect.spec \
	README.md \
	$(NULL)

CLEANFILES += \
	$(nmvpnservice_DATA) \
	$(appdata_files)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

ACLOCAL_AMFLAGS = -I m4
