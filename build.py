import os

buildCommonFile = os.path.join('script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings, ''):
    libraries = ['hermes', 'hmsextmodule', 'hmsextserializer', 'hmsextgzipreader']

    if settings.mBuildTarget == 'android':
        libraries.extend(['hmsextjni', 'hmsextaassetreader'])

    buildMake(libraries, settings, '')
