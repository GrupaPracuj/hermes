@echo off
echo .
echo .
echo ########################################################
echo #                  *** HERMES ***                      #
echo # usage: build.bat 'target' 'debug' 'arch' 'variant'   #
echo # available targets: android, apple, linux             #
echo # debug: debug                                         #
echo # arch: armeabi-v7a, arm64-v8a, x86, x86_64            #
echo # variants: ios, macos                                 #
echo ########################################################
echo .
echo .

IF "%1"=="" (
	echo * target not set          *
	echo * default target: android *
	SET target=-target android
) ELSE (
	SET target=-target %1
)
IF "%2"=="" (
	echo * debug not set *
	echo .
) ELSE (
	SET debug=-%2
)
IF "%3"=="" (
	echo * architecture not set *
	echo .
) ELSE (
	SET arch=-arch %3
)
IF "%4"=="" (
	echo * variant not set *
	echo .
) ELSE (
	SET variant=-variant %4
)

cd .\depend\libressl
python .\build.py -target %target% %variant% %mode% %arch%
cd ..\..\depend\jsoncpp
python .\build.py -target %target% %variant% %mode% %arch%
cd ..\..\depend\aes
python .\build.py -target %target% %variant% %mode% %arch%
cd ..\..\depend\zlib
python .\build.py -target %target% %variant% %mode% %arch%
cd ..\..\depend\curl
python .\build.py -target %target% %variant% %mode% %arch%
cd ..\..\
python .\build.py -target %target% %variant% %mode% %arch%