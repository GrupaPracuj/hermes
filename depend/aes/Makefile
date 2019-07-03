LIBRARY_NAME = libaes.a

SRCFILES = $(wildcard source/*.c)
OBJFILES = $(SRCFILES:%.c=%.o)

CFLAGS += -Iinclude

ifndef NDEBUG
CFLAGS += -g -D_DEBUG=1 -DDEBUG=1
else
ifdef DSYM
CFLAGS += -g
endif
CFLAGS += -DNDEBUG=1 -O2
endif

all: $(LIBRARY_NAME)

$(LIBRARY_NAME): $(OBJFILES)
	$(AR) rs $@ $^

%.d:%.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM -MF $@ $<
    
ifneq ($(MAKECMDGOALS), clean)
-include $(OBJFILES:.o=.d)
endif

clean:
ifeq ($(OS), Windows_NT)
	del $(LIBRARY_NAME) source\*.o source\*.d >nul 2>&1
else
	$(RM) $(LIBRARY_NAME) source/*.o source/*.d
endif

.PHONY: all clean
