import os
import platform
import re
import subprocess
import shutil
import sys

class Settings:
    def __init__(self):
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
    print('Check \'CMake\'...')

    packageVersion = '3.14.4'
    packageName = 'cmake-' + packageVersion
    packageExtension = ''
    applicationDir = ''
    applicationName = ''
    platformName = platform.system().lower()
    destinationDir = os.path.join(pDestinationDir, platformName, 'cmake')

    if platformName == 'linux':
        packageName += '-Linux-x86_64'
        packageExtension = '.tar.gz'
        applicationDir = os.path.join(destinationDir, packageName, 'bin')
        applicationName = 'cmake'
    elif platformName == 'darwin':
        packageName += '-Darwin-x86_64'
        packageExtension = '.tar.gz'
        applicationDir = os.path.join(destinationDir, packageName, 'CMake.app', 'Contents', 'bin')
        applicationName = 'cmake'
    elif platformName == 'windows':
        packageExtension = '.zip'
        if platform.machine().endswith('64'):
            packageName += '-win64-x64'
        else:
            packageName += '-win32-x86'
        applicationDir = os.path.join(destinationDir, packageName, 'bin')
        applicationName = 'cmake'

    result = ''
    applicationPath = os.path.join(destinationDir, applicationName)
    if os.path.isfile(applicationPath):
        result = applicationPath
    elif downloadAndExtract('https://github.com/Kitware/CMake/releases/download/v' + packageVersion + '/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(os.path.join(applicationDir, applicationName)):
        shutil.copy2(os.path.join(applicationDir, applicationName), destinationDir)
        shutil.rmtree(os.path.join(destinationDir, packageName))
        result = applicationPath

    return result
    
def prepareGo(pDestinationDir):
    print('Preparing \'Go\'...')

    status = False
    
    if not os.path.isfile(pDestinationDir):
        packageVersion = '1.11.4'
        packageName = 'go' + packageVersion
        platformName = platform.system()

        if platformName == 'Linux':
            packageName += '.linux-amd64.tar.gz'
        elif platformName == 'Darwin':
            packageName += '.darwin-amd64.tar.gz'
        elif platformName == 'Windows':
            if platform.machine().endswith('64'):
                packageName += '.windows-amd64.zip'
            else:
                packageName += '.windows-386.zip'

        status = downloadAndExtract('https://dl.google.com/go/' + packageName, pDestinationDir, packageName, '')

    return status

def prepareMakeWin32(pDestinationDir):
    print('Preparing \'make\'...')

    destinationMakePath = os.path.join(pDestinationDir, 'make.exe')
    destinationIconvPath = os.path.join(pDestinationDir, 'libiconv2.dll')
    destinationIntlPath = os.path.join(pDestinationDir, 'libintl3.dll')
    
    if not os.path.isfile(destinationMakePath):
        if downloadAndExtract('https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81-bin.zip/download', pDestinationDir, 'make-3.81-bin.zip', 'make-3.81-bin'):
            extractDir = os.path.join(pDestinationDir, 'make-3.81-bin')
            binDir = os.path.join(extractDir, 'bin')
            shutil.copy2(os.path.join(binDir, 'make.exe'), destinationMakePath)
            shutil.rmtree(extractDir)
     
    if not os.path.isfile(destinationIconvPath) or not os.path.isfile(destinationIntlPath):
        if downloadAndExtract('https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81-dep.zip/download', pDestinationDir, 'make-3.81-dep.zip', 'make-3.81-dep'):
            extractDir = os.path.join(pDestinationDir, 'make-3.81-dep')
            binDir = os.path.join(extractDir, 'bin')
            shutil.copy2(os.path.join(binDir, 'libiconv2.dll'), destinationIconvPath)
            shutil.copy2(os.path.join(binDir, 'libintl3.dll'), destinationIntlPath)
            shutil.rmtree(extractDir)

    return os.path.isfile(destinationMakePath) and os.path.isfile(destinationIconvPath) and os.path.isfile(destinationIntlPath)
        
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
    downloadDir = os.path.join(workingDir, pRelativeRootDir, 'download')
    pSettings.mBuildTarget = sys.argv[1]
    
    print(downloadDir)

    if pSettings.mBuildTarget == 'android':
        hostDetected = False
        
        if platformName == 'linux' or platformName == 'darwin':
            hostDetected = True

            if os.path.isfile('/usr/bin/cmake'):
                pSettings.mCMake = '/usr/bin/cmake'
            else:
                pSettings.mCMake = checkCMake(downloadDir)

                if len(pSettings.mCMake) == 0:
                    print('Error: cmake not found.')
                    return False

            if os.path.isfile('/usr/bin/go'):
                pSettings.mGo = '/usr/bin/go'
            else:
                print('Error: go not found.')
                return False

            if os.path.isfile('/usr/bin/make'):
                pSettings.mMake = '/usr/bin/make'
            else:
                print('Error: make not found.')
                return False

            if os.path.isfile('/usr/bin/ninja'):
                pSettings.mNinja = '/usr/bin/ninja'
            else:
                print('Error: ninja not found.')
                return False
        elif platformName == 'windows':
            hostDetected = True
            
            makeDir = os.path.join(downloadDir, 'make')
            if prepareMakeWin32(makeDir):
                pSettings.mMake = os.path.join(makeDir, 'make.exe')
            else:
                print('Error: make.exe not found.')
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
            print('Error: Not supported host platform: ' + platformName + '.')
            return False
    elif pSettings.mBuildTarget == 'linux':
        hostDetected = False
        
        if platformName == 'linux':
            hostDetected = True
            
            if os.path.isfile('/usr/bin/make'):
                pSettings.mMake = '/usr/bin/make'
            else:
                print('Error: /usr/bin/make not found.')
                return False

        if hostDetected:
            pSettings.mArchFlag = ['-m32', '-m64']
            pSettings.mArchName = ['x86', 'x86_64']
            pSettings.mMakeFlag = ['', '']
            pSettings.mMakeFlag = ['', 'ARCH64=1']
        else:
            print('Error: Not supported host platform: ' + platformName + '.')
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
    
    print(output.decode())

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
    
    print(output.decode())

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

def buildMake(pRootDir, pLibraryName, pSettings, pMakeFlag):
    status = False
    if pSettings.mBuildTarget == 'android':
        status = buildMakeAndroid(pRootDir, pLibraryName, pSettings, pMakeFlag)
    elif pSettings.mBuildTarget == 'linux':
        status = buildMakeLinux(pRootDir, pLibraryName, pSettings, pMakeFlag)
    return status
    
def buildMakeAndroid(pRootDir, pLibraryName, pSettings, pMakeFlag):
    print('Building...')
    status = False
    workingDir = os.getcwd()
    platformName = platform.system()
    toolchainDir = os.path.join(pSettings.mAndroidNdkDir, 'toolchains', 'llvm', 'prebuilt')
    
    if platformName == 'Linux':
        executeShellCommand(pSettings.mMake + ' clean')
        toolchainDir = os.path.join(toolchainDir, 'linux-x86_64', 'bin')
    elif platformName == 'Darwin':
        executeShellCommand(pSettings.mMake + ' clean')
        toolchainDir = os.path.join(toolchainDir, 'darwin-x86_64', 'bin')
    elif platformName == 'Windows':
        executeCmdCommand(pSettings.mMake + ' clean', workingDir)
        if platform.machine().endswith('64'):
            toolchainDir = os.path.join(toolchainDir, 'windows-x86_64', 'bin')
        else:
            toolchainDir = os.path.join(toolchainDir, 'windows', 'bin')

    for i in range(0, len(pSettings.mArchName)):
        libraryDir = os.path.join(pRootDir, 'lib', 'android', pSettings.mArchName[i])

        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        else:
            for j in range(0, len(pLibraryName)):
                libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a')
                
                if os.path.isfile(libraryFilepath):
                    os.remove(libraryFilepath)
        
        if os.path.isdir(toolchainDir):
            executablePrefix = os.path.join(toolchainDir, pSettings.mHostTag[i])

            androidApi = pSettings.mAndroidApi
            if (int(androidApi) < 21 and (pSettings.mArchName[i] == 'arm64-v8a' or pSettings.mArchName[i] == 'x86_64')):
                androidApi = '21'
                print('Force Android API: \"21\" for architecture \"' + pSettings.mArchName[i] + '\".')

            os.environ['LD'] = executablePrefix + '-ld'
            os.environ['AR'] = executablePrefix + '-ar'
            os.environ['RANLIB'] = executablePrefix + '-ranlib'
            os.environ['STRIP'] = executablePrefix + '-strip'

            if pSettings.mHostTag[i] == 'arm-linux-androideabi':
                executablePrefix = os.path.join(toolchainDir, 'armv7a-linux-androideabi')

            os.environ['CXX'] = executablePrefix + androidApi + '-clang++'
            os.environ['CC'] = executablePrefix + androidApi + '-clang'

            os.environ['CXXFLAGS'] = pSettings.mArchFlag[i]
            os.environ['CFLAGS'] = pSettings.mArchFlag[i]

            makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
            
            for j in range(0, len(pLibraryName)):
                makeCommand += ' lib' + pLibraryName[j] + '.a'
 
            if len(pSettings.mMakeFlag[i]) > 0:
                makeCommand += ' ' + pSettings.mMakeFlag[i]
                
            if pMakeFlag is not None and len(pMakeFlag) > 0:
                makeCommand += ' ' + pMakeFlag
                
            for j in range(2, len(sys.argv)):
                makeCommand += ' ' + sys.argv[j]
                
            buildSuccess = False

            if platformName == 'Linux' or platformName == 'Darwin':
                buildSuccess = executeShellCommand(makeCommand) == 0
            elif platformName == 'Windows':
                buildSuccess = executeCmdCommand(makeCommand, workingDir) == 0
                
            if buildSuccess:
                for j in range(0, len(pLibraryName)):
                    try:
                        shutil.copy2(os.path.join(workingDir, 'lib' + pLibraryName[j] + '.a'), os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a'))
                    except FileNotFoundError:
                        pass

            if platformName == 'Linux' or platformName == 'Darwin':
                executeShellCommand(pSettings.mMake + ' clean')
            elif platformName == 'Windows':
                executeCmdCommand(pSettings.mMake + ' clean', workingDir)
                
            print('Build status for ' + pSettings.mArchName[i] + ': ' + ('Succeeded' if buildSuccess else 'Failed') + '\n')
            
            status |= buildSuccess
        
    return status
    
def buildMakeLinux(pRootDir, pLibraryName, pSettings, pMakeFlag):
    print('Building...')
    status = False
    workingDir = os.getcwd()

    executeShellCommand(pSettings.mMake + ' clean')

    for i in range(0, len(pSettings.mArchName)):
        libraryDir = os.path.join(pRootDir, 'lib', 'linux', pSettings.mArchName[i])

        if not os.path.isdir(libraryDir):
            os.makedirs(libraryDir)
        else:
            for j in range(0, len(pLibraryName)):
                libraryFilepath = os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a')
                
                if os.path.isfile(libraryFilepath):
                    os.remove(libraryFilepath)
      
        envCXXFLAGS = pSettings.mArchFlag[i]
        envCFLAGS = pSettings.mArchFlag[i]

        os.environ['CXXFLAGS'] = envCXXFLAGS
        os.environ['CFLAGS'] = envCFLAGS

        makeCommand = pSettings.mMake + ' -j' + pSettings.mCoreCount
        
        for j in range(0, len(pLibraryName)):
            makeCommand += ' lib' + pLibraryName[j] + '.a'

        if len(pSettings.mMakeFlag[i]) > 0:
            makeCommand += ' ' + pSettings.mMakeFlag[i]
            
        if pMakeFlag is not None and len(pMakeFlag) > 0:
            makeCommand += ' ' + pMakeFlag
            
        for j in range(2, len(sys.argv)):
            makeCommand += ' ' + sys.argv[j]
            
        buildSuccess = executeShellCommand(makeCommand) == 0
            
        if buildSuccess:
            for j in range(0, len(pLibraryName)):
                try:
                    shutil.copy2(os.path.join(workingDir, 'lib' + pLibraryName[j] + '.a'), os.path.join(libraryDir, 'lib' + pLibraryName[j] + '.a'))
                except FileNotFoundError:
                    pass

        executeShellCommand(pSettings.mMake + ' clean')
            
        print('Build status for ' + pSettings.mArchName[i] + ': ' + ('Succeeded' if buildSuccess else 'Failed') + '\n')
        
        status |= buildSuccess
        
    return status
    
