include ../local.mk
include ../common.mk


OBJECTS = main.o cmdline.o


all: text_align

clean:
	$(RM) $(OBJECTS) cmdline.h cmdline.c text_align

.PHONY: all clean

text_align: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../libtextalign/libtextalign.a

%.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -Wall -Werror -o $@ $<

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -Wall -Werror -o $@ $<

%.c: %.ggo
	$(GENGETOPT) --input="$<"

main.c: cmdline.c
