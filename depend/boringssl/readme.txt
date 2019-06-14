Run 'python3 build.py X NDEBUG=1' where 'X' is a destination platform name eg. 'android', 'linux' etc. Remove 'NDEBUG=1' to build 'debug' version.

TO-DO:
https://github.com/google/boringssl/archive/8f574c3.zip

${HERMES_HOME}/download/darwin/cmake/CMake.app/Contents/bin/cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DANDROID_ABI=armeabi-v7a -DCMAKE_TOOLCHAIN_FILE=${ANDROID_HOME}/ndk-bundle/build/cmake/android.toolchain.cmake -DANDROID_NATIVE_API_LEVEL=18 -DGO_EXECUTABLE=${HERMES_HOME}/download/darwin/go/bin/go -GNinja -DCMAKE_MAKE_PROGRAM=${HERMES_HOME}/download/darwin/ninja/ninja ..
${HERMES_HOME}/darwin/ninja/ninja
