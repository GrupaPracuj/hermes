exec(compile(source = open('build/build_common.py').read(), filename = 'build/build_common.py', mode = 'exec'))

settings = Settings()

if configure('android', settings):
    prepareToolchainAndroid(settings)
    buildMakeAndroid('hermes', settings)
    cleanup(settings)

