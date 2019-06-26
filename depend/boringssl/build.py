import os
import shutil

libraryVersion = '8f574c37dae935a29bff56e750101e5b354503de'
libraryName = 'boringssl-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://github.com/google/boringssl/archive/' + libraryVersion + '.zip', '', libraryName + '.zip', ''):
        remove('include')
        os.chdir(libraryName)
        if buildCMake(['crypto', 'ssl'], settings, '-DGO_EXECUTABLE=' + settings.mGo, False):
            includePath = os.path.join('include', 'openssl')
            shutil.copytree(includePath, os.path.join('..', includePath))
        os.chdir('..')
        remove(libraryName)
