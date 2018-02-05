import os

buildCommonFile = os.path.join('build', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure('linux', settings):
    buildMakeLinux('', ['hermes', 'hmsextmodule'], settings, '')
