import multiprocessing
import os
import platform
import subprocess
import shutil
import sys

class Settings:
    def __init__(self):
        self.mHostName = ''
        self.mAndroidApi = ''
        self.mAndroidNdkDir = ''
        self.mCPU = ''
        self.mWorkingDir = ''
        self.mTmpDir = ''
        self.mToolchainRemovable = False
        self.mToolchainDir = ''
        self.mToolchainArch = []
        self.mToolchainName = []
        self.mArch = []
        self.mArchFlag = []
        self.mArchName = []
        
def configure(pBuildTarget, pSettings):
    pSettings.mHostName = platform.system()
    pSettings.mWorkingDir = os.getcwd()
    pSettings.mTmpDir = os.path.join(pSettings.mWorkingDir, 'tmp')
    
    if pBuildTarget is not None and pBuildTarget == 'android':
        if pSettings.mHostName == 'Linux' or pSettings.mHostName == 'Darwin':
            pSettings.mAndroidApi = os.getenv('HERMES_ANDROID_API')
            pSettings.mAndroidNdkDir = os.getenv('ANDROID_NDK_ROOT')

            if pSettings.mAndroidApi is None:
                pSettings.mAndroidApi = '21'

            if pSettings.mAndroidNdkDir is None or not os.path.isdir(pSettings.mAndroidNdkDir):
                androidHome = os.getenv('ANDROID_HOME')
                
                if androidHome is not None and os.path.isdir(androidHome):
                    pSettings.mAndroidNdkDir = os.path.join(androidHome, 'ndk-bundle')
                    
                    if not os.path.isdir(pSettings.mAndroidNdkDir):
                        print('Error: ' + pSettings.mAndroidNdkDir + ' is not a valid path.')
                        return False
                else:
                    print('Error: Occurred problem related to ANDROID_HOME environment variable.')
                    return False
                    
            pSettings.mToolchainDir = os.path.join(pSettings.mWorkingDir, 'toolchain')
            pSettings.mToolchainArch = ['arm', 'arm', 'arm64', 'mips', 'mips64', 'x86', 'x86_64']
            pSettings.mToolchainName = ['arm-linux-androideabi', 'arm-linux-androideabi', 'aarch64-linux-android', 'mipsel-linux-android', 'mips64el-linux-android', 'i686-linux-android', 'x86_64-linux-android']
            pSettings.mArch = ['android', 'android-armeabi', 'android64-aarch64', 'android-mips', 'linux64-mips64', 'android-x86', 'android64']
            pSettings.mArchFlag = ['-mthumb', '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon', '', '', '', '-march=i686 -m32 -mtune=intel -msse3 -mfpmath=sse", "-march=x86-64 -m64 -mtune=intel -msse4.2 -mpopcnt']
            pSettings.mArchName = ['arm', 'armv7', 'arm64', 'mips', 'mips64', 'x86', 'x86_64']

            print('Android API: "' + pSettings.mAndroidApi + '"')
            print('Android NDK path: "' + pSettings.mAndroidNdkDir + '"')
        else:
            print('Error: Not supported host platform: ' + pSettings.mHostName + '.')
            return False
    else:
        print('Error: Not supported build target and/or host platform.')
        return False
        
    pSettings.mCPU = str(multiprocessing.cpu_count())
        
    print('Available CPU cores: ' + pSettings.mCPU)
    
    return True
         
def prepareToolchainAndroid(pSettings):
    for i in range(0, len(pSettings.mArch)):
        destinationPath = os.path.join(pSettings.mToolchainDir, pSettings.mToolchainArch[i])
        
        if not os.path.isdir(destinationPath):
            os.system(os.path.join(pSettings.mAndroidNdkDir, 'build/tools/make_standalone_toolchain.py') + ' --arch ' + pSettings.mToolchainArch[i] + ' --api ' + str(21) + ' --stl libc++ --install-dir ' + destinationPath)
            
    pSettings.mToolchainRemovable = True
            
    return
    
def cleanup(pSettings):
    if pSettings.mTmpDir is not None and os.path.isdir(pSettings.mTmpDir):
        shutil.rmtree(pSettings.mTmpDir)
        
    if pSettings.mToolchainRemovable and pSettings.mToolchainDir is not None and os.path.isdir(pSettings.mToolchainDir):
        shutil.rmtree(pSettings.mToolchainDir)
        
    return

