-include local.mk
include common.mk

LIBBIO_A = lib/libbio/src/libbio.a


all: $(LIBBIO_A)
	$(MAKE) -C libtextalign
	$(MAKE) -C command_line_aligner
	$(MAKE) -C unit_tests

clean:
	$(MAKE) -C libtextalign clean
	$(MAKE) -C command_line_aligner clean
	-$(MAKE) -C postgresql clean
	$(MAKE) -C python clean
	$(MAKE) -C unit_tests clean

clean-all: clean
	$(MAKE) -C lib/libbio clean

$(LIBBIO_A): lib/libbio/local.mk
	$(MAKE) -C lib/libbio

lib/libbio/local.mk: local.mk
	$(CP) local.mk lib/libbio/local.mk
