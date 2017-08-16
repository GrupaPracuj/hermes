exec(compile(source = open('build_common.py').read(), filename = 'build_common.py', mode = 'exec'))

def buildAndroid(pSettings):
    libraryName = 'hermes'

    for i in range(0, len(pSettings.mArch)):
        toolchainBinDir = os.path.join(os.path.join(os.path.join(pSettings.mToolchainDir, pSettings.mToolchainArch[i]), 'bin'), pSettings.mToolchainName[i])
        os.environ['CXX'] = toolchainBinDir + '-clang++'
        os.environ['LD'] = toolchainBinDir + '-ld'
        os.environ['AR'] = toolchainBinDir + '-ar'
        os.environ['RANLIB'] = toolchainBinDir + '-ranlib'
        os.environ['STRIP'] = toolchainBinDir + '-strip'
        
        envSYSROOT = os.path.join(os.path.join(pSettings.mToolchainDir, pSettings.mToolchainArch[i]), 'sysroot')        
        envCXXFLAGS = pSettings.mArchFlag[i] + ' --sysroot=' + envSYSROOT
        
        if pSettings.mArch[i] == 'linux64-mips64':
            envCXXFLAGS += ' -fintegrated-as'

        os.environ['SYSROOT'] = envSYSROOT
        os.environ['CXXFLAGS'] = envCXXFLAGS
        
        libraryDir = os.path.join(os.path.join(os.path.join(pSettings.mWorkingDir, 'lib'), 'android'), pSettings.mArchName[i])
        libraryFilepath = os.path.join(libraryDir, 'lib' + libraryName + '.a')
        
        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        elif os.path.isfile(libraryFilepath):
            os.remove(libraryFilepath)
            
        if os.path.isdir(pSettings.mTmpDir):
            shutil.rmtree(pSettings.mTmpDir)
            
        os.makedirs(pSettings.mTmpDir)
        
        #if make $1 -j${CPU_CORE}; then
        #    cp ${TMP_DIR}/libhermes.a ${WORKING_DIR}/lib/android/${ARCH_NAME[$i]}/
	    #fi

	    #make clean
        
    return

settings = Settings()
if configure('android', settings):
    prepareToolchainAndroid(settings)
    buildAndroid(settings)
    cleanup(settings)

