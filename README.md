# text_align

The purpose of this library is to provide text alignment for large(ish) constant alphabets; in particular alignment of Unicode code points. Currently, it contains a version of the Smith-Waterman algorithm that attempts to split the dynamic programming matrix into blocks and fill them in parallel.

At this point, the library should be considered experimental.

## Requirements

- GNU Make

### C++ Library

- Compiler that supports C++17, e.g. GCC 8 or Clang 7
- Boost (tested with version 1.68)
- To build the provided command line tool, GNU Gengetopt is also required.
- The supporting library, libbio, requires Ragel at compile time.

### Python Extension

- Python 3 (tested with version 3.6)
- setuptools
- Cython

### PostgreSQL Extension

- Tested with version 10.4

## Building

Running make in the project root builds the C++ library, the command line tool and the unit tests. Running `make clean` cleans everything including the PostgreSQL and Python extensions.

1. Edit local.mk. Useful variables include `CC`, `CXX`, `PYTHON`, `EXTRA_CFLAGS`, `EXTRA_CXXFLAGS` and `EXTRA_LDFLAGS`.
2. Run make.
3. To build either of the extensions, change to the subdirectory in question and run make.
