import os
import sys
sys.path.append('script')
from build_common import *

settings = Settings()

if configure(settings, ''):
    libraries = ['hermes', 'hmsextmodule', 'hmsextserializer', 'hmsextgzipreader']

    if settings.mBuildTarget == 'android':
        libraries.extend(['hmsextjni', 'hmsextaassetreader'])

    buildMake(libraries, settings, '')
