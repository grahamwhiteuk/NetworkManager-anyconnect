project('NetworkManager-anyconnect', 'c', license : 'GPL2', version : '1.2.1')

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

plugindir = join_paths(libdir64, 'NetworkManager/')

conf = configuration_data()

as_version = meson.project_version()
as_name = meson.project_name()

conf.set_quoted('VERSION', as_version)
conf.set_quoted('PACKAGE_VERSION', as_version)
conf.set_quoted('PACKAGE_NAME', as_name)
conf.set_quoted('PACKAGE_TARNAME', as_name)
conf.set_quoted('PACKAGE', as_name)
conf.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/grahamwhiteuk/Networkmanager-anyconnect/issues')
conf.set_quoted('PACKAGE_STRING', as_name + ' ' + as_version)

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

conf.set_quoted('GETTEXT_PACKAGE', as_name)
conf.set_quoted('LOCALEDIR', join_paths(datadir, 'locale'))
conf.set_quoted('NM_ANYCONNECT_LOCALEDIR', join_paths(datadir, 'locale'))
conf.set_quoted('NM_PLUGIN_DIR', join_paths(libdir, 'NetworkManager'))
conf.set_quoted('G_LOG_DOMAIN', 'nm-anyconnect')

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
  conf.set('HAVE_LC_MESSAGES', 1)
endif

if get_option('with-libnm-glib') and not get_option('with_gnome')
  error('Building --with-libnm-glib conflicts with --without-gnome')
endif

libnm = dependency('libnm', required: true)
glib2 = dependency('glib-2.0', required: true)

deps = [libnm, glib2, dlopen]

if get_option('with-gnome')
  conf.set('GDK_VERSION_MIN_REQUIRED', 'GDK_VERSION_3_4')
  gtk3 = dependency('gtk+-3.0', required: true, version: '>= 3.4')
  libnma = dependency('libnma', required: true, version: '>= 1.7.0')
  libsecret = dependency('libsecret-1', required: true, version: '>= 0.18')

  if get_option('with-libnm-glib')
    libnm_gtk = dependency('libnm-gtk', required: true, version: '>= 1.7.0')
    nm = dependency('NetworkManager', required: true, version: '>= 1.7.0')
    libnm_util = dependency('libnm-util', required: true, version: '>= 1.7.0')
    libnm_glib = dependency('libnm-glib', required: true, version: '>= 1.7.0')
    libnm_glib_vpn = dependency('libnm-glib-vpn', required: true, version: '>= 1.7.0')

    conf.set('NM_VERSION_MIN_REQUIRED', 'NM_VERSION_1_2')
    conf.set('NM_VERSION_MAX_ALLOWED', 'NM_VERSION_1_2')

    deps += [libnm_gtk, nm, libnm_util, libnm_glib, libnm_glib_vpn]

  endif

  deps += [gtk3, libnma, libsecret]

endif

conf.set('_GNU_SOURCE', 1)
# Always defined to indicate that i18n is enabled
conf.set('ENABLE_NLS', 1)
conf.set('NETWORKMANAGER_COMPILATION', 'NM_NETWORKMANAGER_COMPILATION_DEFAULT|NM_NETWORKMANAGER_COMPILATION_LIB_EDITOR')

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

if get_option('enable-more-warnings') == 'yes' or get_option('enable-more-warnings') == 'error'
  if cc.has_argument('-Wextra')
    add_project_arguments('-Wextra', language : 'c')
  endif

  if cc.has_argument('-Wall')
    add_project_arguments('-Wall', language : 'c')
  endif

  if get_option('enable-more-warnings') == 'error' and cc.has_argument('-Werror')
    add_project_arguments('-Werror', language : 'c')
  endif
endif

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
conf_nm_service.set('LIBEXECDIR', as_name)
conf_nm_service.set('PLUGINDIR', libdir64)
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
               configuration : conf)
