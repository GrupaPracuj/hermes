import os
import shutil
import sys
sys.path.append(os.path.join('..', '..', 'script'))
from build_common import *

libraryVersion = '7.86.0'
libraryName = 'curl-' + libraryVersion

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://curl.haxx.se/download/' + libraryName + '.zip', '', libraryName + '.zip', ''):
        remove('include')

        enableSSL = os.path.isdir(os.path.join('..', 'libressl', 'include', 'openssl'))
        if enableSSL:
            for i in range(0, len(settings.mArchName)):
                libraryPath = os.path.join(settings.mRootDir, 'lib', settings.mBuildTarget, settings.mArchName[i])
                enableSSL &= os.path.isfile(os.path.join(libraryPath, 'libcrypto.a')) and os.path.isfile(os.path.join(libraryPath, 'libssl.a'))
        flagSSL = ''
        if enableSSL and (settings.mBuildTarget == 'android' or settings.mBuildTarget == 'linux'):
            includeSSL = os.path.join(os.getcwd(), '..', 'libressl', 'include')
            for i in range(0, len(settings.mArchFlagC)):
                if len(settings.mArchFlagC[i]) > 0:
                    settings.mArchFlagC[i] += ' '

                settings.mArchFlagC[i] += '-I' + includeSSL

            flagSSL = '-DCURL_USE_OPENSSL=OFF -DSSL_ENABLED=ON -DUSE_OPENSSL=ON -DHAVE_LIBCRYPTO=ON -DHAVE_LIBSSL=ON'
        elif settings.mBuildTarget == 'apple':
            flagSSL = '-DCURL_USE_SECTRANSP=ON -DCURL_CA_FALLBACK=ON'
        else:
            flagSSL = '-DCURL_USE_OPENSSL=OFF'

        outputLibraryName = 'curl'
        for i in range(1, len(sys.argv)):
            if sys.argv[i] == '-debug':
                outputLibraryName = 'curl-d'
                break

        os.chdir(libraryName)
        if buildCMake(['curl'], settings, flagSSL + ' -DCURL_CA_BUNDLE=\'none\' -DCURL_CA_PATH=\'none\' -DBUILD_SHARED_LIBS=OFF -DHTTP_ONLY=ON -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DENABLE_MANUAL=OFF', False, 'lib', [outputLibraryName]):
            includePath = os.path.join('include', 'curl')
            shutil.copytree(includePath, os.path.join('..', includePath))
            remove(os.path.join('..', includePath, 'Makefile.am'))
            remove(os.path.join('..', includePath, 'Makefile.in'))
        os.chdir('..')
        remove(libraryName)
