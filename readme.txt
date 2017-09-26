

			 _    _ ______ _____  __  __ ______  _____ 
			| |  | |  ____|  __ \|  \/  |  ____|/ ____|
			| |__| | |__  | |__) | \  / | |__  | (___  
			|  __  |  __| |  _  /| |\/| |  __|  \___ \ 
			| |  | | |____| | \ \| |  | | |____ ____) |
			|_|  |_|______|_|  \_\_|  |_|______|_____/ 




DESCRIPTION:

  Simple library helping with network requests, tasks management, logger and file based data storage,
  build with cross-platform environment in mind.

VERSION:

  1.1.0

SUPPORTED PLATFORMS:

  iOS, Android (NDK), macOS (experimental), Linux (partially)

LICENSE:

  zlib

CREDITS:

  Patryk Nadrowski <nadro-linux@wp.pl>
  Sebastian Markowski <kansaidragon@gmail.com>

DEPENDENCIES:

  libcurl library - https://curl.haxx.se/libcurl
  JsonCpp library - https://github.com/open-source-parsers/jsoncpp
  OpenSSL library - https://www.openssl.org
  aes library - https://github.com/BrianGladman/aes
  base64 code - https://github.com/ReneNyffenegger/cpp-base64
  sha1 code - https://github.com/etodd/sha1

QUICK START (iOS):
  * Execute build_ios.sh scripts in $(HERMES_HOME)/depend/jsoncpp, $(HERMES_HOME)/depend/aes 
  	and $(HERMES_HOME)/depend/curl (add 'NDEBUG=1' parameter to build_ios.sh
    to build libraries in the release mode).
  * Open Xcode, go to XCode -> Preferences -> Locations -> Custom Paths and add new entry [Name:
    'HERMES_HOME' Display Name: 'Hermes' Path: 'path to Hermes project root eg. /Users/name/hermes'].
    Next open hermes.xcodeproj and build 'hermes_ios' target.
  * In your project settings add '$(HERMES_HOME)/include $(HERMES_HOME)/depend/jsoncpp/include'
    entry to 'Header Search Paths' and '$(HERMES_HOME)/lib/ios' entry to 'Library Search Paths'.
    Next add libhermes.a, libcurl.a, libjsoncpp.a, libaes.a from Hermes library path and libz.tbd,
    Security.framework from Xcode library set. Make sure that these libraries were added
    to 'Link Binary With Libraries' section in 'Build Phases' project settings). Finally you need
    to switch 'C++ Language Dialect' to 'C++11 [-std=c++11]' or higher and 'C++ Standard Library'
    to 'libc++'.
  * Check 'sample.cpp' for use.
  
QUICK START (Android):
  * Add 'ANDROID_NDK_ROOT' and 'HERMES_HOME' environment variables with destination paths set to AndroidNDK
    and Hermes library directories.
  * If you want to use SSL with curl, execute build_android.sh script in $(HERMES_HOME)/depend/openssl
  * Execute build_android.sh scripts in $(HERMES_HOME)/depend/jsoncpp, $(HERMES_HOME)/depend/aes 
  	and $(HERMES_HOME)/depend/curl (add 'NDEBUG=1' parameter to build_android.sh
    to build libraries in the release mode).
  * Execute build_android.sh from $(HERMES_HOME) directory (add 'NDEBUG=1' parameter to build_android.sh
    to build libraries in the release mode).
  * From Android Studio import $(HERMES_HOME)/example/01.HelloWorld_Android project.

MISCELLANEOUS:
  * This product includes software developed by the OpenSSL Project for use
    in the OpenSSL Toolkit (http://www.openssl.org/)"
  * 'certificate.pem' in '01.HelloWorld_Android' came from 'https://curl.haxx.se/docs/caextract.html'
    and is licensed under MPL 2.0 terms.
