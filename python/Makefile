include ../local.mk
include ../common.mk


GENERATED_SOURCES =	parallel_smith_waterman/alignment_context.cpp

SETUP_PY_CMD =		CC="$(CC)" \
					CXX="$(CXX)" \
					INCLUDE_DIRS="$(INCLUDE_DIRS)" \
					LIB_DIRS="$(LIB_DIRS)" \
					LIBRARIES="$(LIBRARIES)" \
					EXTRA_CXXFLAGS="$(EXTRA_CXXFLAGS)" \
					EXTRA_LDFLAGS="$(EXTRA_LDFLAGS)" \
					$(PYTHON) setup.py

all: build_ext

build_ext:
	$(SETUP_PY_CMD) build_ext --inplace

bdist_wheel:
	$(SETUP_PY_CMD) bdist_wheel

clean:
	$(RM) $(GENERATED_SOURCES)
	$(RM) -r build
