APPLICATION_NAME := 01.HelloWorld

ifneq ($(OS),Windows_NT)
    PLATFORM_NAME := $(shell uname -s)
    ARCHITECTURE_NAME := $(shell uname -p)
    
    ifeq ($(PLATFORM_NAME),Linux)
		ifeq ($(ARCHITECTURE_NAME),x86_64)
			CXXFLAGS += -m64
			LDFLAGS += -L../../lib/linux/x86_64
		else
			CXXFLAGS += -m32
			LDFLAGS += -L../../lib/linux/x86
		endif
		LDFLAGS += -lhmsextserializer -lhermes -laes -lcurl -ljsoncpp -lz -lzlib -lssl -lcrypto -pthread
    endif
    ifeq ($(PLATFORM_NAME),Darwin)
        CXXFLAGS += -m64
		LDFLAGS += -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks \
			-L../../lib/macos/aes.xcframework/macos-arm64_x86_64 -L../../lib/macos/curl.xcframework/macos-arm64_x86_64 \
			-L../../lib/macos/hermes.xcframework/macos-arm64_x86_64 -L../../lib/macos/hmsextgzipreader.xcframework/macos-arm64_x86_64 \
			-L../../lib/macos/hmsextmodule.xcframework/macos-arm64_x86_64 -L../../lib/macos/hmsextserializer.xcframework/macos-arm64_x86_64 \
			-L../../lib/macos/jsoncpp.xcframework/macos-arm64_x86_64 -L../../lib/macos/zlib.xcframework/macos-arm64_x86_64 \
			-framework CoreFoundation -framework Cocoa -framework Security -lhmsextserializer -lhermes -laes -lcurl -ljsoncpp -lzlib
    endif
endif

SRCFILES = $(wildcard *.cpp)
OBJFILES = $(SRCFILES:%.cpp=%.o)

CXXFLAGS += -std=c++17 -fPIC -fno-strict-aliasing -fstack-protector -fvisibility=hidden -fvisibility-inlines-hidden -I. -I../.. -I../../include -I../../depend/jsoncpp/include

ifndef NDEBUG
CXXFLAGS += -g -D_DEBUG=1 -DDEBUG=1
else
CXXFLAGS += -DNDEBUG=1 -O2
endif

all: $(SRCFILES) $(APPLICATION_NAME)
    
$(APPLICATION_NAME): $(OBJFILES) 
	$(CXX) $(OBJFILES) $(LDFLAGS) -o $@

%.d:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MF $@ $<

ifneq ($(MAKECMDGOALS), clean)
-include $(OBJFILES:.o=.d)
endif

clean:
	$(RM) $(APPLICATION_NAME) $(OBJFILES) $(OBJFILES:.o=.d)

.PHONY: all build clean
