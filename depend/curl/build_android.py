import os
import shutil

libraryVersion = '7.59.0'
libraryName = 'curl-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'build', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure('android', settings):
    if downloadAndExtract('https://curl.haxx.se/download/' + libraryName + '.zip', '', libraryName + '.zip', ''):
        prepareToolchainAndroid(settings)
        remove('include')
        shutil.copy2('Makefile.in', os.path.join(libraryName, 'Makefile'))
        openSSL = ('CURL_OPENSSL=1' if os.path.isdir(os.path.join('..', 'openssl', 'include', 'openssl', 'android')) else '')
        os.chdir(libraryName)
        if buildMakeAndroid(os.path.join('..', '..', '..'), ['curl'], settings, openSSL):
            includePath = os.path.join('include', 'curl')
            shutil.copytree(includePath, os.path.join('..', includePath))
            remove(os.path.join('..', includePath, 'Makefile.am'))
            remove(os.path.join('..', includePath, 'Makefile.in'))
        os.chdir('..')
        remove(libraryName)
        cleanup(settings)
