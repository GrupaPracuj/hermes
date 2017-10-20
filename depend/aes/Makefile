LIBRARY_NAME = libaes.a

SRCFILES = $(wildcard source/*.c)
OBJFILES = $(SRCFILES:%.c=%.o)

CFLAGS += -pipe -fPIC -fno-strict-aliasing -fstack-protector -Iinclude

ifndef NDEBUG
CFLAGS += -g -D_DEBUG=1 -DDEBUG=1
else
CFLAGS += -DNDEBUG=1 -O2
endif

all: build

$(LIBRARY_NAME): $(OBJFILES)
	$(AR) rs $@ $^

build: $(LIBRARY_NAME)
	cp $^ tmp/
	rm $^

%.d:%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM -MF $@ $<

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJFILES:.o=.d)
endif

clean:
	$(RM) $(OBJFILES) $(OBJFILES:.o=.d)

.PHONY: all build clean

