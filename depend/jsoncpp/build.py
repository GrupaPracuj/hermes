import os
import shutil

libraryVersion = '1.9.3'
libraryName = 'jsoncpp-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://github.com/open-source-parsers/jsoncpp/archive/' + libraryVersion + '.zip', '', libraryName + '.zip', ''):
        remove('include')
        shutil.copy2('Makefile.in', os.path.join(libraryName, 'Makefile'))
        os.chdir(libraryName)
        if buildMake(['jsoncpp'], settings, ''):
            shutil.copytree(os.path.join('include', 'json'), os.path.join('..', 'include', 'json'))
        os.chdir('..')
        remove(libraryName)
