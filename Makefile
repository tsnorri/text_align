all:
	$(MAKE) -C command_line_aligner
	$(MAKE) -C python

clean:
	$(MAKE) -C command_line_aligner clean
	$(MAKE) -C python clean
