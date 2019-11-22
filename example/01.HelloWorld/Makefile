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
		POSTLDFLAGS := -lssl -lcrypto -pthread
    endif
    ifeq ($(PLATFORM_NAME),Darwin)
        CXXFLAGS += -m64 -fno-aligned-allocation -fvisibility=hidden -fvisibility-inlines-hidden
		LDFLAGS += -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks \
			-L../../lib/macos -framework CoreFoundation -framework Cocoa -framework Security
    endif
endif

SRCFILES = $(wildcard *.cpp)
OBJFILES = $(SRCFILES:%.cpp=%.o)

CXXFLAGS += -std=c++17 -fPIC -fno-strict-aliasing -fstack-protector -I. -I.. -I$(HERMES_HOME) -I$(HERMES_HOME)/include -I$(HERMES_HOME)/depend/jsoncpp/include

LDFLAGS += -lhmsextserializer -lhermes -laes -lcurl -ljsoncpp -lz -lzlib $(POSTLDFLAGS)

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
