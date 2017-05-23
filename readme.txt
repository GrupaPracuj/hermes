

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

  iOS, Linux (partially)

LICENSE:

  MIT

CREDITS:

  Patryk Nadrowski <nadro-linux@wp.pl>
  Sebastian Markowski <kansaidragon@gmail.com>

DEPENDENCIES:

  libcurl - https://curl.haxx.se/libcurl/
  JsonCpp - https://github.com/open-source-parsers/jsoncpp

QUICK START (iOS):
  * Open Xcode, go to XCode -> Preferences -> Locations -> Custom Paths and add new entry [Name:
    'HERMES_HOME' Display Name: 'Hermes' Path: 'path to Hermes project root eg. /Users/name/hermes'].
    Next open hermes.xworkspace and build both hermes and jsoncpp projects. If you want to use
    a custom libcurl library check readme.txt at depend/curl path.
  * In your project settings add '$(HERMES_HOME)/include $(HERMES_HOME)/depend/curl/include
    $(HERMES_HOME)/depend/jsoncpp/include' entry to 'Header Search Paths' and '$(HERMES_HOME)/lib/ios'
    entry to 'Library Search Paths'. Next add following libraries: libhermes.a, libcurl.a
    and libjsoncpp.a (make sure that these libraries were added to 'Link Binary With Libraries'
    section in 'Build Phases' project settings). Finaly you need to switch 'C++ Language Dialect'
    to 'C++11 [-std=c++11]' or higher and 'C++ Standard Library' to 'libc++'.
  * Check 'sample.cpp' for use.
  
