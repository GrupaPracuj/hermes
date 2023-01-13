import os
import sys
sys.path.append(os.path.join('..', '..', 'script'))
from build_common import *

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    buildMake(['zlib'], settings, '')
