import multiprocessing
import os
import platform
import subprocess
import shutil
import sys
import urllib.request
import zipfile

class Settings:
    def __init__(self):
        self.mHostName = ''
        self.mAndroidApi = ''
        self.mAndroidNdkDir = ''
        self.mCoreCount = ''
        self.mMake = ''
        self.mWorkingDir = ''
        self.mBuildDir = ''
        self.mTmpDir = ''
        self.mToolchainRemovable = False
        self.mToolchainDir = ''
        self.mToolchainArch = []
        self.mToolchainName = []
        self.mArch = []
        self.mArchFlag = []
        self.mArchName = []
        
def downloadAndExtract(pURL, pDestinationDir, pFileName, pFileExtension):
    status = False

    outputDir = os.path.join(pDestinationDir, pFileName)
    downloadFile = outputDir + '.' + pFileExtension
    urllib.request.urlretrieve(pURL, downloadFile)
    
    if os.path.isfile(downloadFile):
        archiveFile = zipfile.ZipFile(downloadFile)
        archiveFile.extractall(outputDir)
        archiveFile.close()
        status = True
        
    return status
    
def prepareMake(pDestinationDir):
    status = False

    destinationMakePath = os.path.join(pDestinationDir, 'make.exe')
    destinationIconvPath = os.path.join(pDestinationDir, 'libiconv2.dll')
    destinationIntlPath = os.path.join(pDestinationDir, 'libintl3.dll')
    
    if not os.path.isfile(destinationMakePath):
        if downloadAndExtract('https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81-bin.zip/download', pDestinationDir, 'make-3.81-bin', 'zip'):
            extractDir = os.path.join(pDestinationDir, 'make-3.81-bin')
            binDir = os.path.join(extractDir, 'bin')
            shutil.copy2(os.path.join(binDir, 'make.exe'), destinationMakePath)
            shutil.rmtree(extractDir)
            os.remove(extractDir + '.zip')
     
    if not os.path.isfile(destinationIconvPath) or not os.path.isfile(destinationIntlPath):
        if downloadAndExtract('https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81-dep.zip/download', pDestinationDir, 'make-3.81-dep', 'zip'):
            extractDir = os.path.join(pDestinationDir, 'make-3.81-dep')
            binDir = os.path.join(extractDir, 'bin')
            shutil.copy2(os.path.join(binDir, 'libiconv2.dll'), destinationIconvPath)
            shutil.copy2(os.path.join(binDir, 'libintl3.dll'), destinationIntlPath)
            shutil.rmtree(extractDir)
            os.remove(extractDir + '.zip')

    return os.path.isfile(destinationMakePath) and os.path.isfile(destinationIconvPath) and os.path.isfile(destinationIntlPath)
        
def configure(pBuildTarget, pSettings):
    if sys.version_info < (3, 0):
        print('Error: This script require Python 3.0 or higher.')
        return False

    pSettings.mHostName = platform.system()
    pSettings.mWorkingDir = os.getcwd()
    pSettings.mBuildDir = os.path.join(pSettings.mWorkingDir, 'build')
    pSettings.mTmpDir = os.path.join(pSettings.mWorkingDir, 'tmp')
    
    if pBuildTarget is not None and pBuildTarget == 'android':
        hostDetected = False
        
        if pSettings.mHostName == 'Linux' or pSettings.mHostName == 'Darwin':
            hostDetected = True
            pSettings.mMake = 'make'
        elif pSettings.mHostName == 'Windows':
            hostDetected = True
            
            if prepareMake(pSettings.mBuildDir):
                pSettings.mMake = os.path.join(pSettings.mBuildDir, 'make.exe')
            else:    
                print('Error: make.exe not found.')
                return False
    
        if hostDetected:
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
            pSettings.mArchFlag = ['-mthumb', '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon', '', '', '', '-march=i686 -m32 -mtune=intel -msse3 -mfpmath=sse', '-march=x86-64 -m64 -mtune=intel -msse4.2 -mpopcnt']
            pSettings.mArchName = ['arm', 'armv7', 'arm64', 'mips', 'mips64', 'x86', 'x86_64']

            print('Android API: "' + pSettings.mAndroidApi + '"')
            print('Android NDK path: "' + pSettings.mAndroidNdkDir + '"')
        else:
            print('Error: Not supported host platform: ' + pSettings.mHostName + '.')
            return False
    else:
        print('Error: Not supported build target and/or host platform.')
        return False
        
    pSettings.mCoreCount = str(multiprocessing.cpu_count())
        
    print('Available CPU cores: ' + pSettings.mCoreCount)
    
    return True
         
def prepareToolchainAndroid(pSettings):
    for i in range(0, len(pSettings.mArch)):
        destinationPath = os.path.join(pSettings.mToolchainDir, pSettings.mToolchainArch[i])
        
        if not os.path.isdir(destinationPath):
            os.system(os.path.join(pSettings.mAndroidNdkDir, 'build/tools/make_standalone_toolchain.py') + ' --arch ' + pSettings.mToolchainArch[i] + ' --api ' + pSettings.mAndroidApi + ' --stl libc++ --install-dir ' + destinationPath)
            
    pSettings.mToolchainRemovable = True
            
    return

def executeShellCommand(pCommandLine):
    process = subprocess.Popen(pCommandLine, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = True)        
    output, error = process.communicate()
    returnCode = process.wait()
    
    print(output.decode())
    
    return returnCode
    
def executeCmdCommand(pCommandLine, pWorkingDir):
    commandLine = pCommandLine.encode()    
    commandLine += b"""
    exit
    """

    process = subprocess.Popen(["cmd", "/q", "/k", "echo off"], stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, cwd = pWorkingDir, shell = True)
    process.stdin.write(commandLine)
    output, error = process.communicate()
    returnCode = process.wait()
    
    print(output.decode())
    
    return returnCode
    
def cleanup(pSettings):
    if pSettings.mTmpDir is not None and os.path.isdir(pSettings.mTmpDir):
        shutil.rmtree(pSettings.mTmpDir)
        
    if pSettings.mToolchainRemovable and pSettings.mToolchainDir is not None and os.path.isdir(pSettings.mToolchainDir):
        shutil.rmtree(pSettings.mToolchainDir)
        
    return
    
def buildMakeAndroid(pLibraryName, pSettings):
    for i in range(0, len(pSettings.mToolchainArch)):
        toolchainBinDir = os.path.join(os.path.join(pSettings.mToolchainDir, pSettings.mToolchainArch[i]), 'bin')
        
        if os.path.isdir(toolchainBinDir):
            executablePrefix = os.path.join(toolchainBinDir, pSettings.mToolchainName[i])
        
            os.environ['CXX'] = executablePrefix + '-clang++'
            os.environ['LD'] = executablePrefix + '-ld'
            os.environ['AR'] = executablePrefix + '-ar'
            os.environ['RANLIB'] = executablePrefix + '-ranlib'
            os.environ['STRIP'] = executablePrefix + '-strip'
            
            envSYSROOT = os.path.join(os.path.join(pSettings.mToolchainDir, pSettings.mToolchainArch[i]), 'sysroot')        
            envCXXFLAGS = pSettings.mArchFlag[i] + ' --sysroot=' + envSYSROOT
            
            if pSettings.mArch[i] == 'linux64-mips64':
                envCXXFLAGS += ' -fintegrated-as'

            os.environ['SYSROOT'] = envSYSROOT
            os.environ['CXXFLAGS'] = envCXXFLAGS
            
            libraryDir = os.path.join(os.path.join(os.path.join(pSettings.mWorkingDir, 'lib'), 'android'), pSettings.mArchName[i])
            libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName + '.a')
            
            if not os.path.isdir(libraryDir):
                os.makedirs(libraryDir)
            elif os.path.isfile(libraryFilepath):
                os.remove(libraryFilepath)

            makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
            
            if len(sys.argv) > 1:
                makeCommand += ' ' + sys.argv[1]
                
            buildSuccess = False
            
            if pSettings.mHostName == 'Linux' or pSettings.mHostName == 'Darwin':
                buildSuccess = executeShellCommand(makeCommand) == 0
            elif pSettings.mHostName == 'Windows':
                buildSuccess = executeCmdCommand(makeCommand, pSettings.mWorkingDir) == 0
                
            if buildSuccess:
                shutil.copy2(os.path.join(pSettings.mWorkingDir, 'lib' + pLibraryName + '.a'), libraryFilepath)

            if pSettings.mHostName == 'Linux' or pSettings.mHostName == 'Darwin':
                executeShellCommand(pSettings.mMake + ' clean')
            elif pSettings.mHostName == 'Windows':
                executeCmdCommand(pSettings.mMake + ' clean', pSettings.mWorkingDir)
        
    return

