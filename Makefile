all:
	$(MAKE) -C libtextalign
	$(MAKE) -C command_line_aligner
	$(MAKE) -C unit_tests

clean:
	$(MAKE) -C libtextalign clean
	$(MAKE) -C command_line_aligner clean
	$(MAKE) -C postgresql clean
	$(MAKE) -C python clean
	$(MAKE) -C unit_tests clean
