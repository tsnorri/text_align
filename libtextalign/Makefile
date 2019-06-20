include ../local.mk
include ../common.mk

OBJECTS		=	alignment_graph_builder.o
CFLAGS		+=	-fPIC
CXXFLAGS	+=	-fPIC


all: libtextalign.a

clean:
	$(RM) $(OBJECTS) libtextalign.a
	
libtextalign.a: $(OBJECTS)
	$(AR) rcs $@ $^
