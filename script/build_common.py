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

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/cmake'):
        return '/usr/bin/cmake'

    packageVersion = '3.14.4'
    packageName = 'cmake-' + packageVersion
    packageExtension = ''
    applicationDir = ''
    applicationName = ''
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
        packageName += '-win64-x64'
        packageExtension = '.zip'
        applicationDir = os.path.join(destinationDir, packageName, 'bin')
        applicationName = 'cmake.exe'

    result = ''
    applicationPath = os.path.join(destinationDir, applicationName)
    if os.path.isfile(applicationPath):
        result = applicationPath
    elif downloadAndExtract('https://github.com/Kitware/CMake/releases/download/v' + packageVersion + '/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(os.path.join(applicationDir, applicationName)):
        shutil.copy2(os.path.join(applicationDir, applicationName), destinationDir)
        shutil.rmtree(os.path.join(destinationDir, packageName))
        result = applicationPath
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
    applicationDir = os.path.join(destinationDir, 'go')

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
    applicationPath = os.path.join(destinationDir, 'go', 'bin', applicationName)
    if os.path.isfile(applicationPath):
        result = applicationPath
    elif downloadAndExtract('https://dl.google.com/go/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(applicationPath):
        result = applicationPath
    elif os.path.isdir(applicationDir):
        shutil.rmtree(applicationDir)

    return result

def checkMake(pDestinationDir):
    print('Check \'Make\'...')

    platformName = platform.system().lower()
    if (platformName == 'linux' or platformName == 'darwin') and os.path.isfile('/usr/bin/make'):
        return '/usr/bin/make'

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

    result = ''
    if os.path.isfile(destinationMakePath) and os.path.isfile(destinationIconvPath) and os.path.isfile(destinationIntlPath):
        result = os.path.join(pDestinationDir, 'make.exe')

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
    applicationPath = os.path.join(destinationDir, applicationName)
    if os.path.isfile(applicationPath):
        result = applicationPath
    elif downloadAndExtract('https://github.com/ninja-build/ninja/releases/download/v' + packageVersion + '/' + packageName + packageExtension, destinationDir, packageName + packageExtension, '') and os.path.isfile(applicationPath):
        if platformName == 'linux' or platformName == 'darwin':
            executeShellCommand('chmod +x ' + applicationPath)

        result = applicationPath
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
    downloadDir = os.path.join(workingDir, pRelativeRootDir, 'download')
    pSettings.mBuildTarget = sys.argv[1]
    
    print(downloadDir)

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
            pSettings.mMakeFlag = ['', '']
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
        toolchainDir = os.path.join(toolchainDir, 'windows-x86_64', 'bin')

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
    
