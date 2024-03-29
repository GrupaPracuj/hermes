import os
import shutil
import sys
sys.path.append(os.path.join('..', '..', 'script'))
from build_common import *

libraryVersion = '3.7.0'
libraryName = 'libressl-' + libraryVersion

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
