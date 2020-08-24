i18n = import('i18n')

potfiles_conf = configuration_data()
potfiles_conf.set('SRCDIR', '')
potfiles_conf.set('BUILDDIR', '')

potfiles = configure_file(
  input: 'POTFILES.in',
  output: 'POTFILES',
  configuration: potfiles_conf,
)

i18n.gettext(
  meson.project_name(),
  args: [
    '--add-comments=TRANSLATORS:',
    '--directory=@0@'.format(meson.source_root()),
    '--directory=@0@'.format(meson.build_root()),
    '--files-from=@0@'.format(meson.current_build_dir() / 'POTFILES'),
    '--package-version=@0@'.format(meson.project_version()),
    '--sort-output',
  ],
  preset: 'glib',
)
