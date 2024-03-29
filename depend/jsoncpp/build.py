import os
import shutil
import sys
sys.path.append(os.path.join('..', '..', 'script'))
from build_common import *

libraryVersion = '1.9.5'
libraryName = 'jsoncpp-' + libraryVersion

settings = Settings()

if configure(settings, os.path.join('..', '..')):
    if downloadAndExtract('https://github.com/open-source-parsers/jsoncpp/archive/' + libraryVersion + '.zip', '', libraryName + '.zip', ''):
        remove('include')
        shutil.copy2('Makefile.in', os.path.join(libraryName, 'Makefile'))
        os.chdir(libraryName)
        if buildMake(['jsoncpp'], settings, ''):
            shutil.copytree(os.path.join('include', 'json'), os.path.join('..', 'include', 'json'))
        os.chdir('..')
        remove(libraryName)
