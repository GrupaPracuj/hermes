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

EXT_SERIALIZER_NAME = libhmsextserializer.a
EXT_SERIALIZER_SRCFILES = ext/hmsSerializer.cpp
EXT_SERIALIZER_OBJFILES = $(EXT_SERIALIZER_SRCFILES:%.cpp=%.o)

EXT_GZIPREADER_NAME = libhmsextgzipreader.a
EXT_GZIPREADER_SRCFILES = ext/hmsGZipReader.cpp
EXT_GZIPREADER_OBJFILES = $(EXT_GZIPREADER_SRCFILES:%.cpp=%.o)

CXXFLAGS += -pipe -std=c++14 -fPIC -fno-strict-aliasing -fstack-protector -Iinclude -Idepend/aes/include -Idepend/curl/include -Idepend/jsoncpp/include -Idepend/zlib/include

ifndef NDEBUG
CXXFLAGS += -g -D_DEBUG=1 -DDEBUG=1
else
ifdef DSYM
CXXFLAGS += -g
endif
CXXFLAGS += -DNDEBUG=1 -O2
endif

all: $(LIB_NAME) $(EXT_MODULE_NAME) $(EXT_SERIALIZER_NAME)

$(LIB_NAME): $(LIB_OBJFILES)
	$(AR) rs $@ $^

$(EXT_MODULE_NAME): $(EXT_MODULE_OBJFILES)
	$(AR) rs $@ $^

$(EXT_JNI_NAME): $(EXT_JNI_OBJFILES)
	$(AR) rs $@ $^
	
$(EXT_AASSETREADER_NAME): $(EXT_AASSETREADER_OBJFILES)
	$(AR) rs $@ $^

$(EXT_SERIALIZER_NAME): $(EXT_SERIALIZER_OBJFILES)
	$(AR) rs $@ $^

$(EXT_GZIPREADER_NAME): $(EXT_GZIPREADER_OBJFILES)
	$(AR) rs $@ $^

%.d:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MF $@ $<

clean:
ifeq ($(OS), Windows_NT)
	del $(LIB_NAME) $(EXT_MODULE_NAME) $(EXT_JNI_NAME) $(EXT_AASSETREADER_NAME) $(EXT_SERIALIZER_NAME) $(EXT_GZIPREADER_NAME) source\*.o source\*.d ext\*.o ext\*.d >nul 2>&1
else
	$(RM) $(LIB_NAME) $(EXT_MODULE_NAME) $(EXT_JNI_NAME) $(EXT_AASSETREADER_NAME) $(EXT_SERIALIZER_NAME) $(EXT_GZIPREADER_NAME) source/*.o source/*.d ext/*.o ext/*.d
endif

.PHONY: all clean
