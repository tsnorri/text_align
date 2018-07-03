include ../local.mk
include ../common.mk

OBJECTS = 	matrix_tests.o


all: unit_tests

clean:
	$(RM) $(OBJECTS) unit_tests

unit_tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../libtextalign/libtextalign.a -lboost_unit_test_framework-mt
