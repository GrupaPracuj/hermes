exec(compile(source = open('build_common.py').read(), filename = 'build_common.py', mode = 'exec'))

settings = Settings()

if configure('android', settings):
    prepareToolchainAndroid(settings)
    buildMakeAndroid('hermes', settings)
    cleanup(settings)

