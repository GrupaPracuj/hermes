LIB_NAME = libhermes.a
LIB_SRCFILES = $(wildcard source/*.cpp)
LIB_OBJFILES = $(LIB_SRCFILES:%.cpp=%.o)

EXT_MODULE_NAME = libhmsextmodule.a
EXT_MODULE_SRCFILES = ext/hmsModule.cpp
EXT_MODULE_OBJFILES = $(EXT_MODULE_SRCFILES:%.cpp=%.o)

EXT_JNI_NAME = libhmsextjni.a
EXT_JNI_SRCFILES = ext/hmsJNI.cpp
EXT_JNI_OBJFILES = $(EXT_JNI_SRCFILES:%.cpp=%.o)

EXT_AASSETREADER_NAME = libhmsextaassetreader.a
EXT_AASSETREADER_SRCFILES = ext/hmsAAssetReader.cpp
EXT_AASSETREADER_OBJFILES = $(EXT_AASSETREADER_SRCFILES:%.cpp=%.o)

CXXFLAGS += -pipe -std=c++11 -fPIC -fno-strict-aliasing -fstack-protector -Iinclude -Idepend/aes/include -Idepend/curl/include -Idepend/jsoncpp/include

ifndef NDEBUG
CXXFLAGS += -g -D_DEBUG=1 -DDEBUG=1
else
CXXFLAGS += -DNDEBUG=1 -O2
endif

all: $(LIB_NAME) $(EXT_MODULE_NAME)

$(LIB_NAME): $(LIB_OBJFILES)
	$(AR) rs $@ $^

$(EXT_MODULE_NAME): $(EXT_MODULE_OBJFILES)
	$(AR) rs $@ $^

$(EXT_JNI_NAME): $(EXT_JNI_OBJFILES)
	$(AR) rs $@ $^
	
$(EXT_AASSETREADER_NAME): $(EXT_AASSETREADER_OBJFILES)
	$(AR) rs $@ $^

%.d:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MF $@ $<

clean:
ifeq ($(OS), Windows_NT)
	del $(LIB_NAME) $(EXT_MODULE_NAME) $(EXT_JNI_NAME) $(EXT_AASSETREADER_NAME) source\*.o source\*.d ext\*.o ext\*.d >nul 2>&1
else
	$(RM) $(LIB_NAME) $(EXT_MODULE_NAME) $(EXT_JNI_NAME) $(EXT_AASSETREADER_NAME) source/*.o source/*.d ext/*.o ext/*.d
endif

.PHONY: all clean
