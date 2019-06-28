import os
import shutil

libraryVersion = '7.65.1'
libraryName = 'curl-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://curl.haxx.se/download/' + libraryName + '.zip', '', libraryName + '.zip', ''):
        remove('include')

        enableSSL = os.path.isdir(os.path.join('..', 'boringssl', 'include', 'openssl'))
        if enableSSL:
            for i in range(0, len(settings.mArchName)):
                libraryPath = os.path.join(settings.mRootDir, 'lib', settings.mBuildTarget, settings.mArchName[i])
                enableSSL &= os.path.isfile(os.path.join(libraryPath, 'libcrypto.a')) and os.path.isfile(os.path.join(libraryPath, 'libssl.a'))
        flagSSL = ''
        if enableSSL and (settings.mBuildTarget == 'android' or settings.mBuildTarget == 'linux'):
            flagSSL = '-DCMAKE_USE_OPENSSL=OFF -DSSL_ENABLED=ON -DUSE_OPENSSL=ON -DHAVE_LIBCRYPTO=ON -DHAVE_LIBSSL=ON -DCMAKE_C_FLAGS=-I' + os.path.join(os.getcwd(), '..', 'boringssl', 'include')
        elif settings.mBuildTarget == 'ios' or settings.mBuildTarget == 'macos':
            flagSSL = '-DCMAKE_USE_SECTRANSP=ON'
        else:
            flagSSL = '-DCMAKE_USE_OPENSSL=OFF'

        outputLibraryName = 'curl-d'
        for j in range(2, len(sys.argv)):
            if sys.argv[j] == 'NDEBUG=1':
                outputLibraryName = 'curl'
                break
                
        os.chdir(libraryName)
        if buildCMake(['curl'], settings, flagSSL + ' -DCURL_CA_BUNDLE_SET=OFF -DCURL_CA_FALLBACK=OFF -DCURL_CA_PATH_SET=OFF -DBUILD_SHARED_LIBS=OFF -DHTTP_ONLY=ON -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DENABLE_MANUAL=OFF', False, 'lib', [outputLibraryName]):
            includePath = os.path.join('include', 'curl')
            shutil.copytree(includePath, os.path.join('..', includePath))
            remove(os.path.join('..', includePath, 'Makefile.am'))
            remove(os.path.join('..', includePath, 'Makefile.in'))
        os.chdir('..')
        remove(libraryName)
