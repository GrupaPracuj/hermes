import os

buildCommonFile = os.path.join('build', 'build_common.py')
exec(compile(source = open(buildCommonFile).read(), filename = buildCommonFile, mode = 'exec'))

settings = Settings()

if configure(settings):
    prepareToolchain(settings)
    buildMakeAndroid('', ['hermes', 'hmsextmodule', 'hmsextjni', 'hmsextaassetreader', 'hmsextserializer'] if settings.mBuildTarget == 'android' else ['hermes', 'hmsextmodule', 'hmsextserializer'], settings, '')
    cleanup(settings)
