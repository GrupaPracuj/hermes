import os

buildCommonFile = os.path.join('script', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings):
    prepareToolchain(settings)
    buildMake('', ['hermes', 'hmsextmodule', 'hmsextjni', 'hmsextaassetreader', 'hmsextserializer', 'hmsextgzipreader'] if settings.mBuildTarget == 'android' else ['hermes', 'hmsextmodule', 'hmsextserializer', 'hmsextgzipreader'], settings, '')
    cleanup(settings)
