import os
import shutil

libraryVersion = '1.8.3'
libraryName = 'jsoncpp-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'build', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure('android', settings):
    if downloadAndExtract('https://github.com/open-source-parsers/jsoncpp/archive/' + libraryVersion + '.zip', '', libraryName + '.zip', ''):
        prepareToolchainAndroid(settings)
        remove('include')
        shutil.copy2('Makefile.in', os.path.join(libraryName, 'Makefile'))
        os.chdir(libraryName)
        if buildMakeAndroid(os.path.join('..', '..', '..'), ['jsoncpp'], settings, ''):
            shutil.copytree(os.path.join('include', 'json'), os.path.join('..', 'include', 'json'))
        os.chdir('..')
        remove(libraryName)
        cleanup(settings)
