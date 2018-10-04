import os
import shutil

libraryVersion = '7.61.1'
libraryName = 'curl-' + libraryVersion

buildCommonFile = os.path.join('..', '..', 'build', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings):
    if downloadAndExtract('https://curl.haxx.se/download/' + libraryName + '.zip', '', libraryName + '.zip', ''):
        prepareToolchain(settings)
        remove('include')
        shutil.copy2('Makefile.in', os.path.join(libraryName, 'Makefile'))
        openSSL = os.path.isdir(os.path.join('..', 'openssl', 'include', 'openssl', settings.mBuildTarget))
        if openSSL:
            for i in range(0, min(len(settings.mArchName), len(settings.mToolchainArch))):
                if os.path.isdir(os.path.join(settings.mToolchainDir, settings.mToolchainArch[i], 'bin')):
                    libraryPath = os.path.join('..', '..', 'lib', settings.mBuildTarget, settings.mArchName[i])
                    openSSL &= os.path.isfile(os.path.join(libraryPath, 'libcrypto.a')) and os.path.isfile(os.path.join(libraryPath, 'libssl.a'))
        os.chdir(libraryName)
        if buildMake(os.path.join('..', '..', '..'), ['curl'], settings, 'CURL_OPENSSL=1' if openSSL else ''):
            includePath = os.path.join('include', 'curl')
            shutil.copytree(includePath, os.path.join('..', includePath))
            remove(os.path.join('..', includePath, 'Makefile.am'))
            remove(os.path.join('..', includePath, 'Makefile.in'))
        os.chdir('..')
        remove(libraryName)
        cleanup(settings)
