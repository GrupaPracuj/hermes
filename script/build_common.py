import os
import platform
import re
import subprocess
import shutil
import sys

class Settings:
    def __init__(self):
        self.mRootDir = ''
        self.mBuildTarget = ''
        self.mAndroidApi = ''
        self.mAndroidNdkDir = ''
        self.mCoreCount = ''
        self.mCMake = ''
        self.mGo = ''
        self.mMake = ''
        self.mNinja = ''
        self.mHostTag = []
        self.mArch = []
        self.mArchFlag = []
        self.mArchName = []
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

    packageVersion = '3.14.4'
    packageName = 'cmake-' + packageVersion
    packageExtension = ''
    applicationName = ''
    applicationDestinationPath = ''
    applicationSourcePath = ''
    applicationCopyPath = ''
    destinationDir = os.path.join(pDestinationDir, platformName, 'cmake')

    if platformName == 'linux':
        packageName += '-Linux-x86_64'
        packageExtension = '.tar.gz'
        applicationName = 'cmake'
        applicationDestinationPath = os.path.join(destinationDir, applicationName)
        applicationSourcePath = os.path.join(destinationDir, packageName, 'bin', applicationName)
        applicationCopyPath = applicationSourcePath
    elif platformName == 'darwin':
        packageName += '-Darwin-x86_64'
        packageExtension = '.tar.gz'
        applicationName = 'cmake'
        applicationDestinationPath = os.path.join(destinationDir, 'CMake.app', 'Contents', 'bin', applicationName)
        applicationSourcePath = os.path.join(destinationDir, packageName, 'CMake.app', 'Contents', 'bin', applicationName)
        applicationCopyPath = os.path.join(destinationDir, packageName)
    elif platformName == 'windows':
        packageName += '-win64-x64'
        packageExtension = '.zip'
        applicationName = 'cmake.exe'
        applicationDestinationPath = os.path.join(destinationDir, applicationName)
        applicationSourcePath = os.path.join(destinationDir, packageName, 'bin', applicationName)
        applicationCopyPath = applicationSourcePath

    result = ''
    
    if os.path.isfile(applicationDestinationPath):
        result = applicationDestinationPath
    elif downloadAndExtract('https://github.com/Kitware/CMake/releases/download/v' + packageVersion + '/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(applicationSourcePath):
        if os.path.isfile(applicationCopyPath):
            shutil.copy2(applicationCopyPath, destinationDir)
        else:
            copy_tree(applicationCopyPath, destinationDir)
        shutil.rmtree(os.path.join(destinationDir, packageName))
        result = applicationDestinationPath
    elif os.path.isdir(destinationDir):
        shutil.rmtree(destinationDir)

    return result

def checkGo(pDestinationDir):
    print('Check \'Go\'...')

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/go'):
        return '/usr/bin/go'

    packageVersion = '1.12.6'
    packageName = 'go' + packageVersion
    packageExtension = ''
    applicationName = ''
    destinationDir = os.path.join(pDestinationDir, platformName)

    if platformName == 'linux':
        packageName += '.linux-amd64'
        packageExtension = '.tar.gz'
        applicationName = 'go'
    elif platformName == 'darwin':
        packageName += '.darwin-amd64'
        packageExtension = '.tar.gz'
        applicationName = 'go'
    elif platformName == 'windows':
        packageName += '.windows-amd64'
        packageExtension = '.zip'
        applicationName = 'go.exe'

    result = ''
    applicationDestinationPath = os.path.join(destinationDir, 'go', 'bin', applicationName)
    if os.path.isfile(applicationDestinationPath):
        result = applicationDestinationPath
    elif downloadAndExtract('https://dl.google.com/go/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(applicationDestinationPath):
        result = applicationDestinationPath
    elif os.path.isdir(os.path.join(destinationDir, 'go')):
        shutil.rmtree(os.path.join(destinationDir, 'go'))

    return result

def checkMake(pDestinationDir):
    print('Check \'Make\'...')

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/make'):
        return '/usr/bin/make'

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
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/go'):
        return '/usr/bin/ninja'

    packageVersion = '1.9.0'
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
        
def configure(pSettings, pRelativeRootDir):
    import multiprocessing

    print('Configuring...')

    if sys.version_info < (3, 0):
        print('Error: This script requires Python 3.0 or higher. Please use "python3" command instead of "python".')
        return False
    
    if len(sys.argv) < 2 or sys.argv[1] is None or (sys.argv[1] != 'android' and sys.argv[1] != 'linux'):
        print('Error: Missing or wrong build target. Following targets are supported: "android", "linux".')
        return False

    workingDir = os.getcwd()
    platformName = platform.system().lower()
    pSettings.mRootDir = os.path.join(workingDir, pRelativeRootDir)
    downloadDir = os.path.join(pSettings.mRootDir, 'download')

    print(downloadDir)

    pSettings.mBuildTarget = sys.argv[1]
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

            pSettings.mGo = checkGo(downloadDir)
            if len(pSettings.mGo) == 0:
                print('Error: \'Go\' not found.')
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
                    pSettings.mAndroidNdkDir = os.path.join(androidHome, 'ndk-bundle')
                    
                    if not os.path.isdir(pSettings.mAndroidNdkDir):
                        print('Error: ' + pSettings.mAndroidNdkDir + ' is not a valid path.')
                        return False
                else:
                    print('Error: Occurred problem related to ANDROID_HOME environment variable.')
                    return False

            validNdk = True

            try:
                with open(os.path.join(pSettings.mAndroidNdkDir, 'source.properties'), 'r') as fileSourceProperties:
                    if not re.compile(r'^Pkg\.Desc = Android NDK\nPkg\.Revision = (19|[2-9]\d|[1-9]\d{2,})\.([0-9]+)\.([0-9]+)(-beta([0-9]+))?').match(fileSourceProperties.read()):
                        validNdk = False
            except:
                validNdk = False

            if not validNdk:
                print('Error: Android NDK 19 or newer is required.')
                return False
            
            pSettings.mHostTag = ['arm-linux-androideabi', 'aarch64-linux-android', 'i686-linux-android', 'x86_64-linux-android']
            pSettings.mArch = ['android-armeabi', 'android64-aarch64', 'android-x86', 'android64']
            pSettings.mArchFlag = ['-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon', '', '-march=i686 -m32 -mtune=intel -msse3 -mfpmath=sse', '-march=x86-64 -m64 -mtune=intel -msse4.2 -mpopcnt']
            pSettings.mArchName = ['armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64']
            pSettings.mMakeFlag = ['', 'ARCH64=1', '', 'ARCH64=1']

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

            pSettings.mGo = checkGo(downloadDir)
            if len(pSettings.mGo) == 0:
                print('Error: \'Go\' not found.')
                return False

            pSettings.mNinja = checkNinja(downloadDir)
            if len(pSettings.mNinja) == 0:
                print('Error: \'Ninja\' not found.')
                return False

        if hostDetected:
            pSettings.mArchFlag = ['-m32', '-m64']
            pSettings.mArchName = ['x86', 'x86_64']
            pSettings.mMakeFlag = ['', 'ARCH64=1']
        else:
            print('Error: Not supported host platform: ' + platformName + ' x86-64' if platform.machine().endswith('64') else ' x86')
            return False
    else:
        return False
        
    pSettings.mCoreCount = str(multiprocessing.cpu_count())
        
    print('Available CPU cores: ' + pSettings.mCoreCount)
    
    return True

def executeShellCommand(pCommandLine):
    process = subprocess.Popen(pCommandLine, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = True)        
    output, error = process.communicate()
    returnCode = process.wait()
    
    returnText = output.decode()
    if len(returnText) > 0:
        print(returnText)

    if returnCode != 0:
        print('Error message:\n' + error.decode("utf-8"))
    
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
    
    returnText = output.decode()
    if len(returnText) > 0:
        print(returnText)

    if returnCode != 0:
        print('Error message:\n' + error.decode("utf-8"))
    
    return returnCode
    
def remove(pPath):
    if pPath is not None:
        if os.path.isdir(pPath):
            shutil.rmtree(pPath)
        elif os.path.isfile(pPath):
            os.remove(pPath)
            
    return

def buildCMake(pLibraryName, pSettings, pCMakeFlag, pDSYM):
    print('Building...')
    status = False
    workingDir = os.getcwd()
    platformName = platform.system().lower()
    
    buildDir = os.path.join(workingDir, 'build_cmake')
    remove(buildDir)

    releaseBuild = False
    for j in range(2, len(sys.argv)):
        if sys.argv[j] == 'NDEBUG=1':
            releaseBuild = True

    if (len(pCMakeFlag) > 0):
        pCMakeFlag += ' '

    if releaseBuild:
        if pDSYM:
            pCMakeFlag += '-DCMAKE_BUILD_TYPE=RelWithDebInfo'
        else:
            pCMakeFlag += '-DCMAKE_BUILD_TYPE=MinSizeRel'
    else:
        pCMakeFlag += '-DCMAKE_BUILD_TYPE=Debug'

    for i in range(0, len(pSettings.mArchName)):
        libraryDir = os.path.join(pSettings.mRootDir, 'lib', pSettings.mBuildTarget, pSettings.mArchName[i])

        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        else:
            for j in range(0, len(pLibraryName)):
                libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a')
                
                if os.path.isfile(libraryFilepath):
                    os.remove(libraryFilepath)

        os.makedirs(buildDir)
        os.chdir(buildDir)
        
        buildSuccess = False
        if pSettings.mBuildTarget == 'android':
            buildSuccess = buildCMakeAndroid(i, pSettings, pCMakeFlag)
        elif pSettings.mBuildTarget == 'linux':
            buildSuccess = buildCMakeLinux(i, pSettings, pCMakeFlag)
        
        if buildSuccess:
            if platformName == 'linux' or platformName == 'darwin':
                buildSuccess = executeShellCommand(pSettings.mNinja) == 0
            elif platformName == 'windows':
                buildSuccess = executeCmdCommand(pSettings.mNinja, buildDir) == 0

            for j in range(0, len(pLibraryName)):
                try:
                    shutil.copy2(os.path.join(buildDir, pLibraryName[j], 'lib' + pLibraryName[j] + '.a'), os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a'))
                except FileNotFoundError:
                    pass

        os.chdir('..')
        remove(buildDir)
            
        print('Build status for ' + pSettings.mArchName[i] + ': ' + ('Succeeded' if buildSuccess else 'Failed') + '\n')
        
        status |= buildSuccess
        
    return status

def buildCMakeAndroid(pIndex, pSettings, pCMakeFlag):
    status = False
    platformName = platform.system().lower()

    cmakeToolchainFile = os.path.join(pSettings.mAndroidNdkDir, 'build', 'cmake', 'android.toolchain.cmake')
    if os.path.isfile(cmakeToolchainFile):
        androidApi = pSettings.mAndroidApi
        if (int(androidApi) < 21 and (pSettings.mArchName[pIndex] == 'arm64-v8a' or pSettings.mArchName[pIndex] == 'x86_64')):
            androidApi = '21'
            print('Force Android API: \"21\" for architecture \"' + pSettings.mArchName[pIndex] + '\".')

        cmakeCommand = pSettings.mCMake + ' ' + pCMakeFlag + ' -DANDROID_ABI=' + pSettings.mArchName[pIndex] + ' -DANDROID_NATIVE_API_LEVEL=' + androidApi + ' -DCMAKE_TOOLCHAIN_FILE=' + cmakeToolchainFile + ' -GNinja -DCMAKE_MAKE_PROGRAM=' + pSettings.mNinja + ' ..'

        if platformName == 'linux' or platformName == 'darwin':
            status = executeShellCommand(cmakeCommand) == 0
        elif platformName == 'windows':
            status = executeCmdCommand(cmakeCommand, workingDir) == 0

    return status

def buildCMakeLinux(pIndex, pSettings, pCMakeFlag):
    status = False
    platformName = platform.system().lower()

    cmakeCommand = pSettings.mCMake + ' ' + pCMakeFlag + ' -DANDROID_ABI=' + pSettings.mArchName[pIndex] + ' -DANDROID_NATIVE_API_LEVEL=' + androidApi + ' -DCMAKE_TOOLCHAIN_FILE=' + cmakeToolchainFile + ' -GNinja -DCMAKE_MAKE_PROGRAM=' + pSettings.mNinja + ' ..'

    if platformName == 'linux' or platformName == 'darwin':
        status = executeShellCommand(cmakeCommand) == 0
    elif platformName == 'windows':
        status = executeCmdCommand(cmakeCommand, workingDir) == 0

    return status

def buildMake(pLibraryName, pSettings, pMakeFlag):
    status = False
    workingDir = os.getcwd()
    platformName = platform.system().lower()

    for j in range(2, len(sys.argv)):
        if sys.argv[j] == 'NDEBUG=1':
            if (len(pMakeFlag) > 0):
                pMakeFlag += ' '

            pMakeFlag += 'NDEBUG=1'

    if platformName == 'linux':
        executeShellCommand(pSettings.mMake + ' clean')
    elif platformName == 'darwin':
        executeShellCommand(pSettings.mMake + ' clean')
    elif platformName == 'windows':
        executeCmdCommand(pSettings.mMake + ' clean', workingDir)

    for i in range(0, len(pSettings.mArchName)):
        libraryDir = os.path.join(pSettings.mRootDir, 'lib', pSettings.mBuildTarget, pSettings.mArchName[i])

        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        else:
            for j in range(0, len(pLibraryName)):
                libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a')
                
                if os.path.isfile(libraryFilepath):
                    os.remove(libraryFilepath)

        buildSuccess = False
        if pSettings.mBuildTarget == 'android':
            buildSuccess = buildMakeAndroid(i, pLibraryName, pSettings, pMakeFlag)
        elif pSettings.mBuildTarget == 'linux':
            buildSuccess = buildMakeLinux(i, pLibraryName, pSettings, pMakeFlag)

        if buildSuccess:
            for j in range(0, len(pLibraryName)):
                try:
                    shutil.copy2(os.path.join(workingDir, 'lib' + pLibraryName[j] + '.a'), os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a'))
                except FileNotFoundError:
                    pass

        if platformName == 'linux' or platformName == 'darwin':
            executeShellCommand(pSettings.mMake + ' clean')
        elif platformName == 'windows':
            executeCmdCommand(pSettings.mMake + ' clean', workingDir)
            
        print('Build status for ' + pSettings.mArchName[i] + ': ' + ('Succeeded' if buildSuccess else 'Failed') + '\n')
        
        status |= buildSuccess

    return status
    
def buildMakeAndroid(pIndex, pLibraryName, pSettings, pMakeFlag):
    print('Building...')
    status = False
    workingDir = os.getcwd()
    platformName = platform.system().lower()
    toolchainDir = os.path.join(pSettings.mAndroidNdkDir, 'toolchains', 'llvm', 'prebuilt')
    
    if platformName == 'linux':
        toolchainDir = os.path.join(toolchainDir, 'linux-x86_64', 'bin')
    elif platformName == 'darwin':
        toolchainDir = os.path.join(toolchainDir, 'darwin-x86_64', 'bin')
    elif platformName == 'windows':
        toolchainDir = os.path.join(toolchainDir, 'windows-x86_64', 'bin')

    if os.path.isdir(toolchainDir):
        executablePrefix = os.path.join(toolchainDir, pSettings.mHostTag[pIndex])

        androidApi = pSettings.mAndroidApi
        if (int(androidApi) < 21 and (pSettings.mArchName[pIndex] == 'arm64-v8a' or pSettings.mArchName[pIndex] == 'x86_64')):
            androidApi = '21'
            print('Force Android API: \"21\" for architecture \"' + pSettings.mArchName[pIndex] + '\".')

        os.environ['LD'] = executablePrefix + '-ld'
        os.environ['AR'] = executablePrefix + '-ar'
        os.environ['RANLIB'] = executablePrefix + '-ranlib'
        os.environ['STRIP'] = executablePrefix + '-strip'

        if pSettings.mHostTag[pIndex] == 'arm-linux-androideabi':
            executablePrefix = os.path.join(toolchainDir, 'armv7a-linux-androideabi')

        cxxSuffix = '-clang++'
        ccSuffix = '-clang'
        if platformName == 'windows':
            cxxSuffix += '.cmd'
            ccSuffix += '.cmd'

        os.environ['CXX'] = executablePrefix + androidApi + cxxSuffix
        os.environ['CC'] = executablePrefix + androidApi + ccSuffix

        os.environ['CXXFLAGS'] = pSettings.mArchFlag[pIndex]
        os.environ['CFLAGS'] = pSettings.mArchFlag[pIndex]

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
            status = executeCmdCommand(makeCommand, workingDir) == 0
        
    return status
    
def buildMakeLinux(pIndex, pLibraryName, pSettings, pMakeFlag):
    print('Building...')

    envCXXFLAGS = pSettings.mArchFlag[pIndex]
    envCFLAGS = pSettings.mArchFlag[pIndex]

    os.environ['CXXFLAGS'] = envCXXFLAGS
    os.environ['CFLAGS'] = envCFLAGS

    makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
    
    for j in range(0, len(pLibraryName)):
        makeCommand += ' lib' + pLibraryName[j] + '.a'

    if len(pSettings.mMakeFlag[pIndex]) > 0:
        makeCommand += ' ' + pSettings.mMakeFlag[pIndex]
        
    if len(pMakeFlag) > 0:
        makeCommand += ' ' + pMakeFlag

    return executeShellCommand(makeCommand) == 0    
