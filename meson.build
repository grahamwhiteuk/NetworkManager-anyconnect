project('NetworkManager-anyconnect', 'c',
  license : 'GPL2',
  version : '1.2.3',
  default_options: ['warning_level=2']
)

# Note: according to the following issue
# https://github.com/mesonbuild/meson/issues/3275
# warning_level=1 adds -Wall
# warning_level=2 adds -Wall + -Wextra
# warning_level=3 adds -Wall + -Wextra + -Wpedantic

# Directories
prefix = get_option('prefix')
bindir = get_option('bindir')
datadir = join_paths(prefix, get_option('datadir'))
desktopdir = join_paths(datadir, 'applications')
includedir = get_option('includedir')
libdir = get_option('libdir')
libexecdir = join_paths(prefix, get_option('libexecdir'))
localedir = get_option('localedir')
systemddir = join_paths(datadir, 'dbus-1', 'system.d')
mandir = get_option('mandir')
sbindir = get_option('sbindir')
infodir = get_option('infodir')
sysconfdir = get_option('sysconfdir')
localstatedir = get_option('localstatedir')
sharedstatedir = get_option('sharedstatedir')

if build_machine.cpu_family() == 'x86_64'
  libdir64 = join_paths(prefix, 'lib64/')
else
  libdir64 = libdir
endif

plugindir = join_paths(libdir64, 'NetworkManager')

config_h = configuration_data()

as_version = meson.project_version()
as_name = meson.project_name()

config_h.set_quoted('VERSION', as_version, description: 'Version number of package')
config_h.set_quoted('PACKAGE_VERSION', as_version, description: 'Define to the version of this package')
config_h.set_quoted('PACKAGE_NAME', as_name, description: 'Define to the full name of this package')
config_h.set_quoted('PACKAGE_TARNAME', as_name, description: 'Define to the one symbol short name of this package')
config_h.set_quoted('PACKAGE', as_name, description: 'Name of package')
config_h.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/grahamwhiteuk/Networkmanager-anyconnect/issues', description: 'Define to the address where bug reports for this package should be sent')
config_h.set_quoted('PACKAGE_STRING', as_name + ' ' + as_version, description: 'Define to the full name and version of this package')

glib_compile_resources = find_program('glib-compile-resources', required: true)
file_prog = find_program('file', required : true)
find_prog = find_program('find', required : true)

gettext_progs = [
  'msgfmt',
  'msgmerge',
  'xgettext',
  'sed',
  'gawk',
]
foreach name : gettext_progs
  prog = find_program(name, required: true)
#  set_variable('@0@_prog'.format(name), prog)
endforeach

config_h.set_quoted('GETTEXT_PACKAGE', as_name, description: 'Gettext package')
config_h.set_quoted('LOCALEDIR', join_paths(datadir, 'locale'), description: 'The Locale Directory')
config_h.set_quoted('NM_ANYCONNECT_LOCALEDIR', join_paths(datadir, 'locale'), description: 'The Locale Directory')
config_h.set_quoted('NM_PLUGIN_DIR', plugindir, description: 'Path to the Editor Plugin')
config_h.set_quoted('G_LOG_DOMAIN', 'nm-anyconnect', description: 'The name to use when logging')

cc = meson.get_compiler('c')

dlopen = cc.find_library('dl', required: true)
if not dlopen.found()
  error('Unable to find dlfcn.h')
endif

code = '''#include <locale.h>
int main () {
  return LC_MESSAGES;
}
'''
r = cc.compiles(code, name : 'LC_MESSAGE check')
if r
  config_h.set('HAVE_LC_MESSAGES', 1, description: 'Define if your <locale.h> file defines LC_MESSAGES')
endif

if get_option('with-libnm-glib') and not get_option('with_gnome')
  error('Building --with-libnm-glib conflicts with --without-gnome')
endif

libnm = dependency('libnm', required: true)
glib2 = dependency('glib-2.0', required: true)
libnotify = dependency('libnotify', required: false, version: '>= 0.7.0')

config_h.set10('HAVE_LIBNOTIFY_07', libnotify.found() and libnotify.version().version_compare('>= 0.7'),
           description: 'Define if you have libnotify 0.7 or later')

deps = [libnm, glib2, libnotify, dlopen]

if get_option('with-gnome')
  config_h.set('GDK_VERSION_MIN_REQUIRED', 'GDK_VERSION_3_4', description: 'The minimum required version of GDK')
  gtk3 = dependency('gtk+-3.0', required: true, version: '>= 3.4')
  libnma = dependency('libnma', required: true, version: '>= 1.7.0')

  if get_option('with-libnm-glib')
    libnm_gtk = dependency('libnm-gtk', required: true, version: '>= 1.7.0')
    nm = dependency('NetworkManager', required: true, version: '>= 1.7.0')
    libnm_util = dependency('libnm-util', required: true, version: '>= 1.7.0')
    libnm_glib = dependency('libnm-glib', required: true, version: '>= 1.7.0')
    libnm_glib_vpn = dependency('libnm-glib-vpn', required: true, version: '>= 1.7.0')

    config_h.set('NM_VERSION_MAX_ALLOWED', 'NM_VERSION_1_2', description: 'Max NetworkManager version (only used in tests)')

    deps += [libnm_gtk, nm, libnm_util, libnm_glib, libnm_glib_vpn]

  endif

  deps += [gtk3, libnma]

endif

config_h.set('_GNU_SOURCE', 1, description: 'Required to get definitions of NI_MAXSERV and NI_MAXHOST')
# Always defined to indicate that i18n is enabled
config_h.set('ENABLE_NLS', 1, description: 'always defined to indicate that i18n is enabled')
config_h.set('NETWORKMANAGER_COMPILATION', 'NM_NETWORKMANAGER_COMPILATION_DEFAULT|NM_NETWORKMANAGER_COMPILATION_LIB_EDITOR', description:'Used by the NetworkManager shared utilities')

test_args = [
  '-Wunknown-warning-option',
  '-Wdeclaration-after-statement',
  '-Wfloat-equal',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wimplicit-fallthrough',
  '-Wimplicit-function-declaration',
  '-Winit-self',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-include-dirs',
  '-Wmissing-prototypes',
  '-Wpointer-arith',
  '-Wshadow',
  '-Wshift-negative-value',
  '-Wstrict-prototypes',
  '-Wundef',
  '-Wno-duplicate-decl-specifier',
  '-Wno-format-truncation',
  '-Wno-format-y2k',
  '-Wno-missing-field-initializers',
  '-Wno-pragmas',
  '-Wno-sign-compare',
  '-Wno-unused-but-set-variable',
  '-Wno-unused-parameter',
  '-Wunknown-attributes',
  '-Wtypedef-redefinition',
  '-Warray-bounds',
  '-Wparentheses-equality',
  '-Wunused-value',
  '-Wmissing-braces',
]
cc = meson.get_compiler('c')
foreach arg: test_args
  if cc.has_argument(arg)
    add_project_arguments(arg, language : 'c')
  endif
endforeach

if get_option('enable-lto')
  if cc.has_argument('-flto')
    add_project_arguments('-flto', language : 'c')
  endif
endif

if get_option('enable-ld-gc')
  link_gc = '-Wl,--gc-sections'
  if cc.has_argument('-fdata-sections') and cc.has_argument('-ffunction-sections') and cc.links('int main () { return 0; }', name: link_gc, args: link_gc)
    add_project_arguments('-fdata-sections', language : 'c')
    add_project_arguments('-ffunction-sections', language : 'c')
    add_project_arguments('-Wl,--gc-sections', language : 'c')
  else
    error('Disable garbage collector due to unsupported compiler flags.')
  endif
endif

gnome = import('gnome')

asresources = gnome.compile_resources(
  'nm-anyconnect-dialog.ui', 'properties/gresource.xml',
  source_dir : 'properties',
  c_name : 'as')

# glib-compile-resources
glib_resources_path = join_paths(meson.build_root(), 'properties/')
r = run_command('mkdir', glib_resources_path)
if r.returncode() != 0
  message('Could not compile glib resources:')
  message(r.stderr().strip())
endif

r = run_command('glib-compile-resources', '--target', join_paths(glib_resources_path, 'resources.h'), '--sourcedir=properties/', '--generate-header', 'properties/gresource.xml')
if r.returncode() != 0
  message('Could not compile glib resources:')
  error(r.stderr().strip())
endif

r = run_command('glib-compile-resources', '--target', join_paths(glib_resources_path, 'resources.c'), '--sourcedir=properties/', '--generate-source', 'properties/gresource.xml')
if r.returncode() != 0
  message('Could not compile glib resources:')
  error(r.stderr().strip())
endif

# Generate appdata
conf_nm_service = configuration_data()
conf_nm_service.set('LIBEXECDIR', libexecdir)
configure_file(input : 'nm-anyconnect-service.name.in',
               output : 'nm-anyconnect-service.name',
               configuration : conf_nm_service,
               install: true,
               install_dir: join_paths(prefix, 'lib', 'NetworkManager' , 'VPN/'))

install_data('nm-anyconnect-service.conf', install_dir: systemddir)
install_data('appdata/network-manager-anyconnect.metainfo.xml.in', rename: 'network-manager-anyconnect.metainfo.xml', install_dir: join_paths(datadir, 'metainfo/'))

# Generate the RPM spec file
conf_rpm = configuration_data()
conf_rpm.set('PACKAGE', as_name)
conf_rpm.set('VERSION', as_version)
configure_file(input : 'rpm/NetworkManager-anyconnect.spec.in',
               output : 'NetworkManager-anyconnect.spec',
               configuration : conf_rpm)
install_data('rpm/NetworkManager-anyconnect-fix-perms', install_dir: sbindir)

# SELinux (commented out as can be built directly from the source tree)
# configure_file(input : 'rpm/selinux/NetworkManager-anyconnect-selinux.spec',
#                output : 'NetworkManager-anyconnect-selinux.spec',
#                copy : true)
# configure_file(input : 'rpm/selinux/nm_anyconnect_service.fc',
#                output : 'nm_anyconnect_service_selinux.fc',
#                copy : true)
# configure_file(input : 'rpm/selinux/nm_anyconnect_service.if',
#                output : 'nm_anyconnect_service_selinux.if',
#                copy : true)
# configure_file(input : 'rpm/selinux/nm_anyconnect_service.te',
#                output : 'nm_anyconnect_service_selinux.te',
#                copy : true)

# Compiling Network Manager Service
configuration_inc = include_directories('.')
shared_inc = include_directories('shared/')

nm_anyconnect_service_src = ['src/nm-anyconnect-service.c']

shared_utils_inc = include_directories('shared/nm-utils/')
shared_utils_src = ['shared/nm-utils/nm-shared-utils.c', 'shared/nm-utils/nm-vpn-plugin-utils.c']

nm_anyconnect_service_exe = executable('nm-anyconnect-service', sources: nm_anyconnect_service_src + shared_utils_src, dependencies: deps, include_directories: [configuration_inc, shared_inc, shared_utils_inc], install: true, install_dir: libexecdir)

# Compiling SO libraries
plugin_src = ['properties/nm-anyconnect-editor-plugin.c']

editor_src = [join_paths(glib_resources_path, 'resources.c'), join_paths(glib_resources_path, 'resources.h'), 'properties/nm-anyconnect-editor.c']

nm_vpn_plugin_anyconnect_core_src = ['shared/nm-utils/nm-vpn-plugin-utils.c']

nm_vpn_plugin_anyconnect_so = shared_library('nm-vpn-plugin-anyconnect', sources: nm_vpn_plugin_anyconnect_core_src + plugin_src, dependencies: deps, include_directories: [configuration_inc, shared_inc, shared_utils_inc], install: true, install_dir: plugindir)

nm_vpn_plugin_anyconnect_editor_so = shared_library('nm-vpn-plugin-anyconnect-editor', sources: editor_src, dependencies: deps, include_directories: [configuration_inc, shared_inc, shared_utils_inc], install: true, install_dir: plugindir)

# Adding translations
subdir('po/')

# Generating config.h
configure_file(output : 'config.h',
               configuration : config_h)
