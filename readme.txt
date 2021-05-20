

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

  1.3.0

SUPPORTED PLATFORMS:

  iOS, Android (NDK), macOS, Linux

LICENSE:

  zlib

CREDITS:

  Patryk Nadrowski <nadro-linux@wp.pl>
  Sebastian Markowski <kansaidragon@gmail.com>

DEPENDENCIES:

  libcurl library - https://curl.haxx.se/libcurl
  JsonCpp library - https://github.com/open-source-parsers/jsoncpp
  LibreSSL library - https://www.libressl.org
  aes library - https://github.com/BrianGladman/aes
  zlib library - https://zlib.net
  base64 code - https://github.com/ReneNyffenegger/cpp-base64
  sha1 code - https://github.com/etodd/sha1
  
QUICK START (Android):
  * Add 'ANDROID_HOME' environment variable with destination path set to AndroidSDK directory.
  * If you want to use SSL with curl, execute 'python3 ./build.py android NDEBUG=1' command in $(HERMES_HOME)/depend/libressl
  * Execute 'python3 ./build.py android' command in $(HERMES_HOME)/depend/jsoncpp, $(HERMES_HOME)/depend/aes,
    $(HERMES_HOME)/depend/zlib and $(HERMES_HOME)/depend/curl (add 'NDEBUG=1' parameter to build.py
    to build libraries in the release mode -> 'python3 ./build.py android NDEBUG=1').
  * Execute 'python3 ./build.py android' from $(HERMES_HOME) directory (add 'NDEBUG=1' parameter to build.py
    to build libraries in the release mode -> 'python3 ./build.py android NDEBUG=1').
  * From Android Studio import, build and run $(HERMES_HOME)/example/01.HelloWorld_Android project.

QUICK START (Linux):
  * If you want to use SSL with curl, execute 'python3 ./build.py linux NDEBUG=1' command in $(HERMES_HOME)/depend/libressl
  * Execute 'python3 ./build.py linux' command in $(HERMES_HOME)/depend/jsoncpp, $(HERMES_HOME)/depend/aes,
    $(HERMES_HOME)/depend/zlib and $(HERMES_HOME)/depend/curl (add 'NDEBUG=1' parameter to build.py
    to build libraries in the release mode -> 'python3 ./build.py linux NDEBUG=1').
  * Execute 'python3 ./build.py linux' from $(HERMES_HOME) directory (add 'NDEBUG=1' parameter to build.py
    to build libraries in the release mode -> 'python3 ./build.py linux NDEBUG=1').
  * Execute 'make' in $(HERMES_HOME)/example/01.HelloWorld directory and run '01.HelloWorld' application.

QUICK START (iOS / macOS):
  * Execute 'python3 ./build.py apple' command in $(HERMES_HOME)/depend/jsoncpp, $(HERMES_HOME)/depend/aes,
    $(HERMES_HOME)/depend/zlib and $(HERMES_HOME)/depend/curl (add 'NDEBUG=1' parameter to build.py
    to build libraries in the release mode -> 'python3 ./build.py apple NDEBUG=1').
  * Execute 'python3 ./build.py apple' from $(HERMES_HOME) directory (add 'NDEBUG=1' parameter to build.py
    to build libraries in the release mode -> 'python3 ./build.py apple NDEBUG=1').
  * (iOS) From Xcode open, build and run $(HERMES_HOME)/example/01.HelloWorld_iOS project.
  * (macos) Execute 'make' in $(HERMES_HOME)/example/01.HelloWorld directory and run '01.HelloWorld' application.

MISCELLANEOUS:
  * 'certificate.pem' in '01.HelloWorld' and '01.HelloWorld_Android' came from 'https://curl.haxx.se/docs/caextract.html'
    and is licensed under MPL 2.0 terms.
