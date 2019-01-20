import os

buildCommonFile = os.path.join('..', '..', 'script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings):
    buildMake(os.path.join('..', '..'), ['zlib'], settings, '')
