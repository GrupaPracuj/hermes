import os
import platform
import re
import subprocess
import shutil
import sys

class Settings:
    def __init__(self):
        self.mRootDir = ''
        self.mLibDir = ''
        self.mBuildTarget = ''
        self.mBuildVariant = ''
        self.mAndroidApi = ''
        self.mAndroidNdkDir = ''
        self.mAppleSdkDir = ''
        self.mCoreCount = ''
        self.mCMake = ''
        self.mMake = ''
        self.mNinja = ''
        self.mHostTag = []
        self.mArch = []
        self.mArchFlagASM = []
        self.mArchFlagC = []
        self.mArchFlagCXX = []
        self.mArchName = []
        self.mPlatformName = []
        self.mTargetSdk = []
        self.mMakeFlag = []
        
def downloadAndExtract(pURL, pDestinationDir, pFileName, pExtractDir):
    import io
    import ssl
    import tarfile
    import urllib.error
    import urllib.request
    import zipfile
    
    print('Downloading and extracting...')

    status = False
    downloadContent = None

    try:
        context = ssl._create_unverified_context()
        downloadContent = urllib.request.urlopen(pURL, context = context).read()
    except:
        print('Failed')
    
    if downloadContent is not None and len(downloadContent) > 0:
        if pFileName.endswith('.zip'):
            archiveFile = zipfile.ZipFile(io.BytesIO(downloadContent), 'r')
            archiveFile.extractall(os.path.join(pDestinationDir, pExtractDir))
            archiveFile.close()
            status = True
        elif pFileName.endswith('.tar.gz'):
            archiveFile = tarfile.open(fileobj = io.BytesIO(downloadContent), mode = 'r:gz')
            archiveFile.extractall(os.path.join(pDestinationDir, pExtractDir))
            archiveFile.close()
            status = True
        
    return status

def checkCMake(pDestinationDir):
    from distutils.dir_util import copy_tree

    print('Check \'CMake\'...')

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/cmake'):
        return '/usr/bin/cmake'

    packageVersion = '3.20.2'
    packageName = 'cmake-' + packageVersion
    packageExtension = ''
    applicationName = ''
    applicationDestinationPath = ''
    applicationSourcePath = ''    
    destinationDir = os.path.join(pDestinationDir, platformName, 'cmake')

    if platformName == 'linux':
        packageName += '-linux-x86_64'
        packageExtension = '.tar.gz'
        applicationName = 'cmake'
        applicationDestinationPath = os.path.join(destinationDir, 'bin', applicationName)
        applicationSourcePath = os.path.join(destinationDir, packageName, 'bin', applicationName)
        applicationCopyPath = os.path.join(destinationDir, packageName)
    elif platformName == 'darwin':
        packageName += '-macos-universal'
        packageExtension = '.tar.gz'
        applicationName = 'cmake'
        applicationDestinationPath = os.path.join(destinationDir, 'CMake.app', 'Contents', 'bin', applicationName)
        applicationSourcePath = os.path.join(destinationDir, packageName, 'CMake.app', 'Contents', 'bin', applicationName)
        applicationCopyPath = os.path.join(destinationDir, packageName)
    elif platformName == 'windows':
        packageName += '-windows-x86_64'
        packageExtension = '.zip'
        applicationName = 'cmake.exe'
        applicationDestinationPath = os.path.join(destinationDir, 'bin', applicationName)
        applicationSourcePath = os.path.join(destinationDir, packageName, 'bin', applicationName)
        applicationCopyPath = applicationSourcePath

    result = ''
    
    sourceDir = os.path.join(destinationDir, packageName)
    if os.path.isfile(applicationDestinationPath):
        result = applicationDestinationPath
    elif downloadAndExtract('https://github.com/Kitware/CMake/releases/download/v' + packageVersion + '/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(applicationSourcePath):
        copy_tree(sourceDir, destinationDir)
        shutil.rmtree(sourceDir)
        result = applicationDestinationPath
    elif os.path.isdir(destinationDir):
        shutil.rmtree(destinationDir)

    return result

def checkMake(pDestinationDir):
    print('Check \'Make\'...')

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin'):
        if os.path.isfile('/usr/bin/make'):
            return '/usr/bin/make'
        else:
            return ''

    destinationDir = os.path.join(pDestinationDir, platformName, 'make')

    destinationMakePath = os.path.join(destinationDir, 'make.exe')
    destinationIconvPath = os.path.join(destinationDir, 'libiconv2.dll')
    destinationIntlPath = os.path.join(destinationDir, 'libintl3.dll')
    
    if not os.path.isfile(destinationMakePath):
        if downloadAndExtract('https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81-bin.zip/download', destinationDir, 'make-3.81-bin.zip', 'make-3.81-bin'):
            extractDir = os.path.join(destinationDir, 'make-3.81-bin')
            binDir = os.path.join(extractDir, 'bin')
            shutil.copy2(os.path.join(binDir, 'make.exe'), destinationMakePath)
            shutil.rmtree(extractDir)
     
    if not os.path.isfile(destinationIconvPath) or not os.path.isfile(destinationIntlPath):
        if downloadAndExtract('https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81-dep.zip/download', destinationDir, 'make-3.81-dep.zip', 'make-3.81-dep'):
            extractDir = os.path.join(destinationDir, 'make-3.81-dep')
            binDir = os.path.join(extractDir, 'bin')
            shutil.copy2(os.path.join(binDir, 'libiconv2.dll'), destinationIconvPath)
            shutil.copy2(os.path.join(binDir, 'libintl3.dll'), destinationIntlPath)
            shutil.rmtree(extractDir)

    result = ''
    if os.path.isfile(destinationMakePath) and os.path.isfile(destinationIconvPath) and os.path.isfile(destinationIntlPath):
        result = os.path.join(destinationDir, 'make.exe')
    elif os.path.isdir(destinationDir):
        shutil.rmtree(destinationDir)

    return result

def checkNinja(pDestinationDir):
    print('Check \'Ninja\'...')

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/ninja'):
        return '/usr/bin/ninja'

    packageVersion = '1.10.2'
    packageName = 'ninja'
    packageExtension = ''
    applicationName = ''
    destinationDir = os.path.join(pDestinationDir, platformName, 'ninja')

    if platformName == 'linux':
        packageName += '-linux'
        packageExtension = '.zip'
        applicationName = 'ninja'
    elif platformName == 'darwin':
        packageName += '-mac'
        packageExtension = '.zip'
        applicationName = 'ninja'
    elif platformName == 'windows':
        packageName += '-win'
        packageExtension = '.zip'
        applicationName = 'ninja.exe'

    result = ''
    applicationDestinationPath = os.path.join(destinationDir, applicationName)
    if os.path.isfile(applicationDestinationPath):
        result = applicationDestinationPath
    elif downloadAndExtract('https://github.com/ninja-build/ninja/releases/download/v' + packageVersion + '/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(applicationDestinationPath):
        if platformName == 'linux' or platformName == 'darwin':
            executeShellCommand('chmod +x ' + applicationDestinationPath)

        result = applicationDestinationPath
    elif os.path.isdir(destinationDir):
        shutil.rmtree(destinationDir)

    return result
        
def configure(pSettings, pRelativeRootDir, pRelativeLibDir = ''):
    import multiprocessing

    print('Configuring...')

    if sys.version_info < (3, 0):
        print('Error: This script requires Python 3.0 or higher. Please use "python3" command instead of "python".')
        return False
        
    workingDir = os.getcwd()
    if workingDir.find(' ') != -1:
        print('Error: Spaces in a project directory path are not allowed.')
        return False

    for i in range(1, len(sys.argv) - 1):
        if sys.argv[i] == '-target':
            pSettings.mBuildTarget = sys.argv[i + 1].lower()
        elif sys.argv[i] == '-variant':
            pSettings.mBuildVariant = sys.argv[i + 1].lower()
    
    if pSettings.mBuildTarget != 'android' and pSettings.mBuildTarget != 'linux' and pSettings.mBuildTarget != 'apple':
        print('Error: Missing or wrong build target. Following targets are supported: "android", "linux", "apple".')
        return False

    platformName = platform.system().lower()
    pSettings.mRootDir = os.path.join(workingDir, pRelativeRootDir)
    downloadDir = os.path.join(pSettings.mRootDir, 'download')
    if len(pRelativeLibDir) == 0:
        pSettings.mLibDir = pSettings.mRootDir
    else:
        pSettings.mLibDir = os.path.join(workingDir, pRelativeLibDir)

    if pSettings.mBuildTarget == 'android':
        hostDetected = False
        
        if (platformName == 'linux' or platformName == 'darwin' or platformName == 'windows') and platform.machine().endswith('64'):
            hostDetected = True

            pSettings.mMake = checkMake(downloadDir)
            if len(pSettings.mMake) == 0:
                print('Error: \'Make\' not found.')
                return False

            pSettings.mCMake = checkCMake(downloadDir)
            if len(pSettings.mCMake) == 0:
                print('Error: \'CMake\' not found.')
                return False

            pSettings.mNinja = checkNinja(downloadDir)
            if len(pSettings.mNinja) == 0:
                print('Error: \'Ninja\' not found.')
                return False

        if hostDetected:
            androidApi = os.getenv('HERMES_ANDROID_API')
           
            name, separator, version = None, None, None
            
            if androidApi is not None:
                name, separator, version = androidApi.partition('-')
            
            if version is not None and len(version) > 0:
                pSettings.mAndroidApi = version
            else:
                pSettings.mAndroidApi = name
            
            pSettings.mAndroidNdkDir = os.getenv('ANDROID_NDK_ROOT')

            if pSettings.mAndroidApi is None:
                pSettings.mAndroidApi = '21'

            if pSettings.mAndroidNdkDir is None or not os.path.isdir(pSettings.mAndroidNdkDir):
                androidHome = os.getenv('ANDROID_HOME')
                
                if androidHome is not None and os.path.isdir(androidHome):
                    pSettings.mAndroidNdkDir = os.path.join(androidHome, 'ndk')
                    if os.path.isdir(pSettings.mAndroidNdkDir):
                        ndkVersions = [d for d in os.listdir(pSettings.mAndroidNdkDir) if os.path.isdir(os.path.join(pSettings.mAndroidNdkDir, d))]

                        if len(ndkVersions) > 0:
                            ndkVersions.sort(reverse = True)
                            pSettings.mAndroidNdkDir = os.path.join(pSettings.mAndroidNdkDir, ndkVersions[0])
                        else:
                            print('Error: System couldn\'t find any NDK version in ' + pSettings.mAndroidNdkDir + ' directory.')
                            return False
                    else:
                        pSettings.mAndroidNdkDir = os.path.join(androidHome, 'ndk-bundle')
                        
                        if not os.path.isdir(pSettings.mAndroidNdkDir):
                            print('Error: System couldn\'t find NDK in ' + pSettings.mAndroidNdkDir + ' directory.')
                            return False
                else:
                    print('Error: Occurred problem related to ANDROID_HOME environment variable.')
                    return False

            validNdk = True

            try:
                with open(os.path.join(pSettings.mAndroidNdkDir, 'source.properties'), 'r') as fileSourceProperties:
                    if not re.compile(r'^Pkg\.Desc = Android NDK\nPkg\.Revision = (2[2-9]|[3-9]\d|[1-9]\d{2,})\.([0-9]+)\.([0-9]+)(-beta([0-9]+))?').match(fileSourceProperties.read()):
                        validNdk = False
            except:
                validNdk = False

            if not validNdk:
                print('Error: Android NDK 22 or newer is required.')
                return False
            
            pSettings.mHostTag = ['arm-linux-androideabi', 'aarch64-linux-android', 'i686-linux-android', 'x86_64-linux-android']
            pSettings.mArch = ['android-armeabi', 'android64-aarch64', 'android-x86', 'android64']
            pSettings.mArchFlagASM = ['-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon', '', '-march=i686 -m32 -msse3 -mfpmath=sse', '-march=x86-64 -m64 -msse4.2 -mpopcnt']
            pSettings.mArchFlagC = ['-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon -fPIC -fno-strict-aliasing -fstack-protector', '-fPIC -fno-strict-aliasing -fstack-protector', '-march=i686 -m32 -msse3 -mfpmath=sse -fPIC -fno-strict-aliasing -fstack-protector', '-march=x86-64 -m64 -msse4.2 -mpopcnt -fPIC -fno-strict-aliasing -fstack-protector']
            pSettings.mArchFlagCXX = ['-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon -fPIC -fno-strict-aliasing -fstack-protector', '-fPIC -fno-strict-aliasing -fstack-protector', '-march=i686 -m32 -msse3 -mfpmath=sse -fPIC -fno-strict-aliasing -fstack-protector', '-march=x86-64 -m64 -msse4.2 -mpopcnt -fPIC -fno-strict-aliasing -fstack-protector']
            pSettings.mArchName = ['armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64']
            pSettings.mPlatformName = ['', '', '', '']
            pSettings.mMakeFlag = ['DSYM=1', 'ARCH64=1 DSYM=1', 'DSYM=1', 'ARCH64=1 DSYM=1']

            print('Android API: "' + pSettings.mAndroidApi + '"')
            print('Android NDK path: "' + pSettings.mAndroidNdkDir + '"')
        else:
            print('Error: Not supported host platform: ' + platformName + ' x86-64' if platform.machine().endswith('64') else ' x86')
            return False
    elif pSettings.mBuildTarget == 'linux':
        hostDetected = False
        
        if platformName == 'linux' and platform.machine().endswith('64'):
            hostDetected = True
            
            pSettings.mMake = checkMake(downloadDir)
            if len(pSettings.mMake) == 0:
                print('Error: \'Make\' not found.')
                return False

            pSettings.mCMake = checkCMake(downloadDir)
            if len(pSettings.mCMake) == 0:
                print('Error: \'CMake\' not found.')
                return False

            pSettings.mNinja = checkNinja(downloadDir)
            if len(pSettings.mNinja) == 0:
                print('Error: \'Ninja\' not found.')
                return False

        if hostDetected:
            commonFlags = ' -fPIC -fno-strict-aliasing -fstack-protector -fvisibility=hidden'
        
            pSettings.mArchFlagASM = ['-m32', '-m64']
            pSettings.mArchFlagC = ['-m32' + commonFlags, '-m64' + commonFlags]
            pSettings.mArchFlagCXX = ['-m32 -fvisibility-inlines-hidden' + commonFlags, '-m64 -fvisibility-inlines-hidden' + commonFlags]
            pSettings.mArchName = ['x86', 'x86_64']
            pSettings.mPlatformName = ['', '']
            pSettings.mMakeFlag = ['', 'ARCH64=1']
        else:
            print('Error: Not supported host platform: ' + platformName + ' x86-64' if platform.machine().endswith('64') else ' x86')
            return False
    elif pSettings.mBuildTarget == 'apple':
        hostDetected = False
        
        if len(pSettings.mBuildVariant) > 0 and pSettings.mBuildVariant != 'ios' and pSettings.mBuildVariant != 'macos':
            print('Warning: Wrong build variant. Following variants of target "' + pSettings.mBuildTarget + '" are supported: "ios", "macos".');
            pSettings.mBuildVariant = ''
        
        if platformName == 'darwin' and platform.machine().endswith('64'):
            hostDetected = True

            pSettings.mAppleSdkDir = receiveShellOutput('xcode-select --print-path').rstrip()
            if not os.path.isdir(pSettings.mAppleSdkDir):
                print('Error: \'Xcode\' not found.')
                return False

            xcodeValid = False
            xcodeVersion = re.split(' |\.', receiveShellOutput('xcodebuild -version'))
            if len(xcodeVersion) > 1:
                try:
                    if int(xcodeVersion[1]) >= 12:
                        xcodeValid = True
                except ValueError:
                    pass

            if not xcodeValid:
                print('Error: Xcode 12.0 or newer is required.')
                return False

            pSettings.mMake = checkMake(downloadDir)
            if len(pSettings.mMake) == 0:
                print('Error: \'Make\' not found.')
                return False

            pSettings.mCMake = checkCMake(downloadDir)
            if len(pSettings.mCMake) == 0:
                print('Error: \'CMake\' not found.')
                return False

            pSettings.mNinja = checkNinja(downloadDir)
            if len(pSettings.mNinja) == 0:
                print('Error: \'Ninja\' not found.')
                return False

        if hostDetected:
            iOScommonFlags = ' -D__IPHONE_OS_VERSION_MIN_REQUIRED=120400 -gdwarf-2 -fPIC -fno-strict-aliasing -fstack-protector -fvisibility=hidden'
            iOSdeviceFlags = ' -miphoneos-version-min=12.4'
            iOSsimulatorFlags = ' -mios-simulator-version-min=12.4'
            macOScommonFlags = ' -gdwarf-2 -fPIC -fno-strict-aliasing -fstack-protector -fvisibility=hidden'

            pSettings.mArch = ['arm64', 'arm64', 'x86_64', 'arm64', 'x86_64']
            pSettings.mArchFlagASM = ['-arch arm64' + iOSdeviceFlags, '-arch arm64' + iOSsimulatorFlags, '-arch x86_64' + iOSsimulatorFlags, '-arch arm64 -mmacosx-version-min=11.0', '-arch x86_64 -mmacosx-version-min=10.13']
            pSettings.mArchFlagC = ['-arch arm64 -ObjC' + iOScommonFlags + iOSdeviceFlags, '-arch arm64 -ObjC' + iOScommonFlags + iOSsimulatorFlags, '-arch x86_64 -ObjC' + iOScommonFlags + iOSsimulatorFlags, '-arch arm64 -ObjC -mmacosx-version-min=11.0' + macOScommonFlags, '-arch x86_64 -ObjC -mmacosx-version-min=10.13' + macOScommonFlags]
            pSettings.mArchFlagCXX = ['-arch arm64 -ObjC++ -stdlib=libc++ -fvisibility-inlines-hidden' + iOScommonFlags + iOSdeviceFlags, '-arch arm64 -ObjC++ -stdlib=libc++ -fvisibility-inlines-hidden' + iOScommonFlags + iOSsimulatorFlags, '-arch x86_64 -ObjC++ -stdlib=libc++ -fvisibility-inlines-hidden' + iOScommonFlags + iOSsimulatorFlags, '-arch arm64 -ObjC++ -stdlib=libc++ -fvisibility-inlines-hidden -mmacosx-version-min=11.0' + macOScommonFlags, '-arch x86_64 -ObjC++ -stdlib=libc++ -fvisibility-inlines-hidden -mmacosx-version-min=10.13' + macOScommonFlags]
            pSettings.mArchName = pSettings.mArch
            pSettings.mPlatformName = ['ios', 'ios-simulator', 'ios-simulator', 'macos', 'macos']
            pSettings.mMakeFlag = ['ARCH64=1 DSYM=1', 'ARCH64=1 DSYM=1', 'ARCH64=1 DSYM=1', 'ARCH64=1 DSYM=1', 'ARCH64=1 DSYM=1']
            pSettings.mTargetSdk = ['iPhoneOS', 'iPhoneSimulator', 'iPhoneSimulator', '', '']

            print('Toolchain path: "' + pSettings.mAppleSdkDir + '"')
        else:
            print('Error: Not supported host platform: ' + platformName + ' arm64/x86-64' if platform.machine().endswith('64') else ' x86')
            return False
    else:
        return False
        
    pSettings.mCoreCount = str(multiprocessing.cpu_count())
        
    print('Available CPU cores: ' + pSettings.mCoreCount)
    
    return True

def executeShellCommand(pCommandLine, pShowOutput = True):
    process = subprocess.Popen(pCommandLine, stderr = subprocess.PIPE, shell = True)
    output, error = process.communicate()
    returnCode = process.wait()
    
    if pShowOutput:
        if returnCode != 0 and len(error) > 0:
            print('\nError message:\n' + error.decode("utf-8"))
    
    return returnCode

def receiveShellOutput(pCommandLine):
    process = subprocess.Popen(pCommandLine, stdout = subprocess.PIPE, shell = True)
    output, error = process.communicate()
    process.wait()
    
    return output.decode()
    
def executeCmdCommand(pCommandLine, pWorkingDir, pShowOutput = True):
    commandLine = pCommandLine.encode()    
    commandLine += b"""
    exit
    """

    process = subprocess.Popen(["cmd", "/q", "/k", "echo off"], stdin = subprocess.PIPE, stderr = subprocess.PIPE, cwd = pWorkingDir, shell = True)
    process.stdin.write(commandLine)
    output, error = process.communicate()
    returnCode = process.wait()
    
    if pShowOutput:
        if returnCode != 0 and len(error) > 0:
            print('Error message:\n' + error.decode("utf-8"))
    
    return returnCode
    
def remove(pPath):
    if pPath is not None:
        if os.path.isdir(pPath):
            shutil.rmtree(pPath)
        elif os.path.isfile(pPath):
            os.remove(pPath)
            
    return

def buildCMake(pLibraryName, pSettings, pCMakeFlag, pDSYM, pOutputDir, pOutputLibraryName):
    print('Building...')
    status = False
    workingDir = os.getcwd()
    platformName = platform.system().lower()
    
    buildDir = os.path.join(workingDir, 'build_cmake')
    remove(buildDir)

    releaseBuild = True
    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-debug':
            releaseBuild = False
            break

    if (len(pCMakeFlag) > 0):
        pCMakeFlag += ' '

    configType = ''
    if releaseBuild:
        if pDSYM:
            configType = 'RelWithDebInfo'
        else:
            configType = 'MinSizeRel'
    else:
        configType = 'Debug'

    pCMakeFlag += '-DCMAKE_BUILD_TYPE=' + configType

    for i in range(0, len(pSettings.mArchName)):
        libraryDir = os.path.join(pSettings.mLibDir, 'lib', pSettings.mBuildTarget, pSettings.mPlatformName[i], pSettings.mArchName[i])

        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        else:
            for j in range(0, len(pLibraryName)):
                libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a')
                
                if os.path.isfile(libraryFilepath):
                    os.remove(libraryFilepath)

        os.makedirs(buildDir)
        os.chdir(buildDir)

        cmakeFlag = pCMakeFlag
        if len(pSettings.mArchFlagASM[i]) > 0:
            cmakeFlag += ' \"-DCMAKE_ASM_FLAGS=' + pSettings.mArchFlagASM[i] + '\"'
        if len(pSettings.mArchFlagC[i]) > 0:
            cmakeFlag += ' \"-DCMAKE_C_FLAGS=' + pSettings.mArchFlagC[i] + '\"'
        if len(pSettings.mArchFlagCXX[i]) > 0:
            cmakeFlag += ' \"-DCMAKE_CXX_FLAGS=' + pSettings.mArchFlagCXX[i] + '\"'
        
        buildSuccess = False
        if pSettings.mBuildTarget == 'android':
            buildSuccess = buildCMakeAndroid(i, pSettings, cmakeFlag)
        elif pSettings.mBuildTarget == 'linux':
            buildSuccess = buildCMakeLinux(i, pSettings, cmakeFlag)
        elif pSettings.mBuildTarget == 'apple':
            buildSuccess = buildCMakeApple(i, pSettings, cmakeFlag)
        
        if buildSuccess:
            buildCommand = pSettings.mCMake + ' --build . --config ' + configType
            if platformName == 'linux' or platformName == 'darwin':
                buildSuccess = executeShellCommand(buildCommand) == 0
            elif platformName == 'windows':
                buildSuccess = executeCmdCommand(buildCommand, buildDir) == 0

            if (len(pLibraryName) == len(pOutputLibraryName)):
                for j in range(0, len(pLibraryName)):
                    try:
                        shutil.copy2(os.path.join(buildDir, pLibraryName[j] if len(pOutputDir) == 0 else pOutputDir, 'lib' + pOutputLibraryName[j] + '.a'), os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a'))
                    except FileNotFoundError:
                        print('Error: system couldn\'t copy library')
                        pass
            else:
                print('Error: system couldn\'t copy library')

        os.chdir('..')
        remove(buildDir)
            
        print('\nBuild status for ' + pSettings.mArchName[i] + (' (' + pSettings.mPlatformName[i] + ')' if len(pSettings.mPlatformName[i]) > 0 else '') + ': ' + ('Succeeded' if buildSuccess else 'Failed') + '\n')
        
        status |= buildSuccess

    createXCFramework(pLibraryName, pSettings)
        
    return status

def buildCMakeAndroid(pIndex, pSettings, pCMakeFlag):
    status = False
    platformName = platform.system().lower()

    toolchainPath = os.path.join(pSettings.mAndroidNdkDir, 'build', 'cmake', 'android.toolchain.cmake')
    if os.path.isfile(toolchainPath):
        androidApi = pSettings.mAndroidApi
        if (int(androidApi) < 21 and (pSettings.mArchName[pIndex] == 'arm64-v8a' or pSettings.mArchName[pIndex] == 'x86_64')):
            androidApi = '21'
            print('Force Android API: \"21\" for architecture \"' + pSettings.mArchName[pIndex] + '\".')

        cmakeCommand = pSettings.mCMake + ' ' + pCMakeFlag + ' -DANDROID_ABI=' + pSettings.mArchName[pIndex] + ' -DANDROID_NATIVE_API_LEVEL=' + androidApi + ' -DCMAKE_TOOLCHAIN_FILE=' + toolchainPath + ' -GNinja -DCMAKE_MAKE_PROGRAM=' + pSettings.mNinja + ' ..'

        if platformName == 'linux' or platformName == 'darwin':
            status = executeShellCommand(cmakeCommand) == 0
        elif platformName == 'windows':
            status = executeCmdCommand(cmakeCommand, os.getcwd()) == 0

    return status

def buildCMakeLinux(pIndex, pSettings, pCMakeFlag):
    status = False
    platformName = platform.system().lower()

    toolchainPath = ''
    if pSettings.mBuildTarget == 'linux':
        if pSettings.mArchName[pIndex] == 'x86':
            toolchainPath = ' -DCMAKE_TOOLCHAIN_FILE=' + os.path.join(pSettings.mRootDir, 'script', 'linux.toolchain.cmake')

    cmakeCommand = pSettings.mCMake + ' ' + pCMakeFlag + toolchainPath + ' -GNinja -DCMAKE_MAKE_PROGRAM=' + pSettings.mNinja + ' ..'

    if platformName == 'linux':
        status = executeShellCommand(cmakeCommand) == 0

    return status

def buildCMakeApple(pIndex, pSettings, pCMakeFlag):
    status = False
    platformName = platform.system().lower()

    cmakeCommand = ''
    if (pSettings.mPlatformName[pIndex] == 'ios' or pSettings.mPlatformName[pIndex] == 'ios-simulator') and pSettings.mBuildVariant != 'macos':
        toolchainPath = os.path.join(pSettings.mRootDir, 'script', 'ios.toolchain.cmake')
        executableDir = os.path.join(pSettings.mAppleSdkDir, 'Toolchains', 'XcodeDefault.xctoolchain', 'usr', 'bin')
        sysrootDir = os.path.join(pSettings.mAppleSdkDir, 'Platforms', pSettings.mTargetSdk[pIndex] + '.platform', 'Developer', 'SDKs', pSettings.mTargetSdk[pIndex] + '.sdk')
        if os.path.isfile(toolchainPath) and os.path.isdir(executableDir) and os.path.isdir(sysrootDir):
            cmakeCommand = pSettings.mCMake + ' ' + pCMakeFlag + ' -DCMAKE_TOOLCHAIN_FILE=' + toolchainPath + ' -DHMS_XCODE_PATH=' + pSettings.mAppleSdkDir + ' -DHMS_TARGET=' + pSettings.mTargetSdk[pIndex] + ' -GNinja -DCMAKE_MAKE_PROGRAM=' + pSettings.mNinja + ' ..'
    elif pSettings.mPlatformName[pIndex] == 'macos' and pSettings.mBuildVariant != 'ios':
        toolchainPath = os.path.join(pSettings.mRootDir, 'script', 'macos.toolchain.cmake')
        if os.path.isfile(toolchainPath):
            cmakeCommand = pSettings.mCMake + ' ' + pCMakeFlag + ' -DCMAKE_TOOLCHAIN_FILE=' + toolchainPath + ' -DHMS_ARCH=' + pSettings.mArch[pIndex] + ' -GNinja -DCMAKE_MAKE_PROGRAM=' + pSettings.mNinja + ' ..'

    if platformName == 'darwin' and len(cmakeCommand) > 0:
        status = executeShellCommand(cmakeCommand) == 0

    return status

def buildMake(pLibraryName, pSettings, pMakeFlag):
    status = False
    workingDir = os.getcwd()
    platformName = platform.system().lower()
    
    releaseBuild = True
    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-debug':
            releaseBuild = False
            break

    if releaseBuild:
        if (len(pMakeFlag) > 0):
            pMakeFlag += ' '

        pMakeFlag += 'NDEBUG=1'

    if platformName == 'linux' or platformName == 'darwin':
        executeShellCommand(pSettings.mMake + ' clean')
    elif platformName == 'windows':
        executeCmdCommand(pSettings.mMake + ' clean', workingDir)

    for i in range(0, len(pSettings.mArchName)):
        libraryDir = os.path.join(pSettings.mLibDir, 'lib', pSettings.mBuildTarget, pSettings.mPlatformName[i], pSettings.mArchName[i])

        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        else:
            for j in range(0, len(pLibraryName)):
                libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a')
                
                if os.path.isfile(libraryFilepath):
                    os.remove(libraryFilepath)

        os.environ['CFLAGS'] = pSettings.mArchFlagC[i]
        os.environ['CXXFLAGS'] = pSettings.mArchFlagCXX[i]

        buildSuccess = False
        if pSettings.mBuildTarget == 'android':
            buildSuccess = buildMakeAndroid(i, pLibraryName, pSettings, pMakeFlag)
        elif pSettings.mBuildTarget == 'linux':
            buildSuccess = buildMakeLinux(i, pLibraryName, pSettings, pMakeFlag)
        elif pSettings.mBuildTarget == 'apple':
            buildSuccess = buildMakeApple(i, pLibraryName, pSettings, pMakeFlag)

        del os.environ['CFLAGS']
        del os.environ['CXXFLAGS']

        if buildSuccess:
            for j in range(0, len(pLibraryName)):
                try:
                    shutil.copy2(os.path.join(workingDir, 'lib' + pLibraryName[j] + '.a'), os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a'))
                except FileNotFoundError:
                    print('Error: system couldn\'t copy library')
                    pass

        if platformName == 'linux' or platformName == 'darwin':
            executeShellCommand(pSettings.mMake + ' clean')
        elif platformName == 'windows':
            executeCmdCommand(pSettings.mMake + ' clean', workingDir)

        print('\nBuild status for ' + pSettings.mArchName[i] + (' (' + pSettings.mPlatformName[i] + ')' if len(pSettings.mPlatformName[i]) > 0 else '') + ': ' + ('Succeeded' if buildSuccess else 'Failed') + '\n')
        
        status |= buildSuccess

    createXCFramework(pLibraryName, pSettings)

    return status
    
def buildMakeAndroid(pIndex, pLibraryName, pSettings, pMakeFlag):
    print('Building...')
    status = False
    platformName = platform.system().lower()
    toolchainDir = os.path.join(pSettings.mAndroidNdkDir, 'toolchains', 'llvm', 'prebuilt')
    
    if platformName == 'linux':
        toolchainDir = os.path.join(toolchainDir, 'linux-x86_64', 'bin')
    elif platformName == 'darwin':
        toolchainDir = os.path.join(toolchainDir, 'darwin-x86_64', 'bin')
    elif platformName == 'windows':
        toolchainDir = os.path.join(toolchainDir, 'windows-x86_64', 'bin')

    if os.path.isdir(toolchainDir):
        llvmPrefix = os.path.join(toolchainDir, 'llvm')
        if pSettings.mHostTag[pIndex] != 'arm-linux-androideabi':
            clangPrefix = os.path.join(toolchainDir, pSettings.mHostTag[pIndex])
        else:
            clangPrefix = os.path.join(toolchainDir, 'armv7a-linux-androideabi')

        androidApi = pSettings.mAndroidApi
        if (int(androidApi) < 21 and (pSettings.mArchName[pIndex] == 'arm64-v8a' or pSettings.mArchName[pIndex] == 'x86_64')):
            androidApi = '21'
            print('Force Android API: \"21\" for architecture \"' + pSettings.mArchName[pIndex] + '\".')

        os.environ['LD'] = llvmPrefix + '-ld'
        os.environ['AR'] = llvmPrefix + '-ar'
        os.environ['RANLIB'] = llvmPrefix + '-ranlib'
        os.environ['STRIP'] = llvmPrefix + '-strip'

        cxxSuffix = '-clang++'
        ccSuffix = '-clang'
        if platformName == 'windows':
            cxxSuffix += '.cmd'
            ccSuffix += '.cmd'

        os.environ['CXX'] = clangPrefix + androidApi + cxxSuffix
        os.environ['CC'] = clangPrefix + androidApi + ccSuffix

        makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
        
        for j in range(0, len(pLibraryName)):
            makeCommand += ' lib' + pLibraryName[j] + '.a'

        if len(pSettings.mMakeFlag[pIndex]) > 0:
            makeCommand += ' ' + pSettings.mMakeFlag[pIndex]

        if len(pMakeFlag) > 0:
            makeCommand += ' ' + pMakeFlag
            
        if len(pMakeFlag) > 0:
            makeCommand += ' ' + pMakeFlag

        if platformName == 'linux' or platformName == 'darwin':
            status = executeShellCommand(makeCommand) == 0
        elif platformName == 'windows':
            status = executeCmdCommand(makeCommand, os.getcwd()) == 0

        del os.environ['CC']
        del os.environ['CXX']
        del os.environ['LD']
        del os.environ['AR']
        del os.environ['RANLIB']
        del os.environ['STRIP']
        
    return status

def buildMakeLinux(pIndex, pLibraryName, pSettings, pMakeFlag):
    print('Building...')
    status = False
    platformName = platform.system().lower()

    makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
    
    for j in range(0, len(pLibraryName)):
        makeCommand += ' lib' + pLibraryName[j] + '.a'

    if len(pSettings.mMakeFlag[pIndex]) > 0:
        makeCommand += ' ' + pSettings.mMakeFlag[pIndex]
        
    if len(pMakeFlag) > 0:
        makeCommand += ' ' + pMakeFlag

    if platformName == 'linux':
        status = executeShellCommand(makeCommand) == 0

    return status

def buildMakeApple(pIndex, pLibraryName, pSettings, pMakeFlag):
    print('Building...')
    status = False
    platformName = platform.system().lower()

    makeExecute = False
    if (pSettings.mPlatformName[pIndex] == 'ios' or pSettings.mPlatformName[pIndex] == 'ios-simulator') and pSettings.mBuildVariant != 'macos':
        executableDir = os.path.join(pSettings.mAppleSdkDir, 'Toolchains', 'XcodeDefault.xctoolchain', 'usr', 'bin')
        sysrootDir = os.path.join(pSettings.mAppleSdkDir, 'Platforms', pSettings.mTargetSdk[pIndex] + '.platform', 'Developer', 'SDKs', pSettings.mTargetSdk[pIndex] + '.sdk')
        if os.path.isdir(executableDir) and os.path.isdir(sysrootDir):
            os.environ['CC'] = os.path.join(executableDir, 'clang')
            os.environ['CXX'] = os.path.join(executableDir, 'clang++')
            os.environ['LD'] = os.path.join(executableDir, 'ld')
            os.environ['AR'] = os.path.join(executableDir, 'ar')
            os.environ['RANLIB'] = os.path.join(executableDir, 'ranlib')
            os.environ['STRIP'] = os.path.join(executableDir, 'strip')
            os.environ['CFLAGS'] = os.environ['CFLAGS'] + ' -isysroot ' + sysrootDir
            os.environ['CXXFLAGS'] = os.environ['CXXFLAGS'] + ' -isysroot ' + sysrootDir
            makeExecute = True
    if pSettings.mPlatformName[pIndex] == 'macos' and pSettings.mBuildVariant != 'ios':
        makeExecute = True

    if makeExecute:
        makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
        
        for j in range(0, len(pLibraryName)):
            makeCommand += ' lib' + pLibraryName[j] + '.a'

        if len(pSettings.mMakeFlag[pIndex]) > 0:
            makeCommand += ' ' + pSettings.mMakeFlag[pIndex]

        if len(pMakeFlag) > 0:
            makeCommand += ' ' + pMakeFlag

        if platformName == 'darwin' and makeExecute:
            status = executeShellCommand(makeCommand) == 0

        if pSettings.mPlatformName[pIndex] == 'ios' or pSettings.mPlatformName[pIndex] == 'ios-simulator':
            del os.environ['CC']
            del os.environ['CXX']
            del os.environ['LD']
            del os.environ['AR']
            del os.environ['RANLIB']
            del os.environ['STRIP']
        
    return status

def createXCFramework(pLibraryName, pSettings):
    if pSettings.mBuildTarget == 'apple':
        libraryDir = os.path.join(pSettings.mLibDir, 'lib', pSettings.mBuildTarget)

        uniquePlatformNames = []
        for j in range(0, len(pSettings.mPlatformName)):
            if not pSettings.mPlatformName[j] in uniquePlatformNames:
                uniquePlatformNames.append(pSettings.mPlatformName[j])

        for i in range(0, len(pLibraryName)):
            remove(os.path.join(libraryDir, pLibraryName[i] + '.xcframework'))

            for j in range(0, len(uniquePlatformNames)):
                platformDir = os.path.join(libraryDir, uniquePlatformNames[j])

                command = 'lipo -create -output ' + os.path.join(platformDir, 'lib' + pLibraryName[i] + '.a')
                parameters = ''

                archNames = [d for d in os.listdir(platformDir) if os.path.isdir(os.path.join(platformDir, d))]
                for k in range(0, len(archNames)):
                    archDir = os.path.join(platformDir, archNames[k])
                    archFile = os.path.join(archDir, 'lib' + pLibraryName[i] + '.a')
                    if os.path.isfile(archFile):
                        parameters += ' ' + archFile

                if len(parameters) > 0:
                    executeShellCommand(command + parameters)

            command = 'xcodebuild -create-xcframework -output ' + os.path.join(libraryDir, pLibraryName[i] + '.xcframework')
            parameters = ''

            for j in range(0, len(uniquePlatformNames)):
                platformDir = os.path.join(libraryDir, uniquePlatformNames[j])
                platformFile = os.path.join(platformDir, 'lib' + pLibraryName[i] + '.a')
                if os.path.isfile(platformFile):
                    parameters += ' -library ' + platformFile

            if len(parameters) > 0:
                executeShellCommand(command + parameters)

        for i in range(0, len(uniquePlatformNames)):
            remove(os.path.join(libraryDir, uniquePlatformNames[i]))
