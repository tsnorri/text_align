all:
	$(MAKE) -C libtextalign
	$(MAKE) -C command_line_aligner
	$(MAKE) -C python

clean:
	$(MAKE) -C libtextalign clean
	$(MAKE) -C command_line_aligner clean
	$(MAKE) -C python clean
