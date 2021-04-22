import os
import shutil

libraryVersion = 'ecc18ad5ec857f2511b6bdd3f06e7584ae95fb21'#'7.74.0'
libraryName = 'ecc18ad5ec857f2511b6bdd3f06e7584ae95fb21'

buildCommonFile = os.path.join('..', '..', 'script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://codeload.github.com/curl/curl/zip/' + libraryName, '', libraryName + '.zip', ''):
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

            flagSSL = '-DCMAKE_USE_OPENSSL=OFF -DSSL_ENABLED=ON -DUSE_OPENSSL=ON -DHAVE_LIBCRYPTO=ON -DHAVE_LIBSSL=ON'
        elif settings.mBuildTarget == 'ios' or settings.mBuildTarget == 'macos':
            flagSSL = '-DCMAKE_USE_SECTRANSP=ON -DCURL_CA_FALLBACK=ON'
        else:
            flagSSL = '-DCMAKE_USE_OPENSSL=OFF'

        outputLibraryName = 'curl-d'
        for j in range(2, len(sys.argv)):
            if sys.argv[j] == 'NDEBUG=1':
                outputLibraryName = 'curl'
                break
                
        os.chdir('curl-' + libraryName)
        if buildCMake(['curl'], settings, flagSSL + ' -DCURL_CA_BUNDLE=\'none\' -DCURL_CA_PATH=\'none\' -DBUILD_SHARED_LIBS=OFF -DHTTP_ONLY=ON -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DENABLE_MANUAL=OFF', False, 'lib', [outputLibraryName]):
            includePath = os.path.join('include', 'curl')
            shutil.copytree(includePath, os.path.join('..', includePath))
            remove(os.path.join('..', includePath, 'Makefile.am'))
            remove(os.path.join('..', includePath, 'Makefile.in'))
        os.chdir('..')
        remove(libraryName)
