import os
import shutil

libraryVersion = '3.3.3'
libraryName = 'libressl-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/' + libraryName + '.tar.gz', '', libraryName + '.tar.gz', ''):
        remove('include')
        os.chdir(libraryName)
        if buildCMake(['crypto', 'ssl'], settings, '-DLIBRESSL_SKIP_INSTALL=ON -DLIBRESSL_APPS=OFF -DLIBRESSL_TESTS=OFF', False, '', ['crypto', 'ssl']):
            includePath = os.path.join('include', 'openssl')
            shutil.copytree(includePath, os.path.join('..', includePath))
            remove(os.path.join('..', includePath, 'Makefile.am'))
            remove(os.path.join('..', includePath, 'Makefile.in'))
        os.chdir('..')
        remove(libraryName)
