include ../local.mk
include ../common.mk

PG_CONFIG	= pg_config
PG_CPPFLAGS	:= $(CPPFLAGS)
SHLIB_LINK	:= $(LDFLAGS) -L../libtextalign -ltextalign -L../lib/libbio/src -lbio

EXTENSION	= text_align
MODULE_big	= text_align
OBJS		= text_align.o
PGXS		:= $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)

override CC = $(CXX)
