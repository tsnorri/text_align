include ../local.mk
include ../common.mk


OBJECTS = aligner_tests.o
#CFLAGS += -fprofile-arcs -ftest-coverage
#CXXFLAGS += -fprofile-arcs -ftest-coverage


all: unit_tests

clean:
	$(RM) $(OBJECTS) unit_tests

unit_tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../libtextalign/libtextalign.a ../lib/libbio/src/libbio.a -lboost_unit_test_framework$(BOOST_LIBRARY_SUFFIX)
