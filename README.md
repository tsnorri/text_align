# text_align

The purpose of this library is to provide text alignment for large(ish) constant alphabets; in particular alignment of Unicode code points. Currently, it contains a version of the Smith-Waterman algorithm that attempts to split the dynamic programming matrix into blocks and fill them in parallel.

## Requirements

- GNU Make

### C++ Library

- Compiler that supports C++17, e.g. GCC 7 or Clang 6
- Boost (tested with version 1.67)
- To build the provided command line tool, GNU Gengetopt is also required.

### Python Extension

- Python 3 (tested with version 3.6)
- setuptools
- Cython

### PostgreSQL Extension

- Tested with version 10.4

## Building

1. Edit local.mk. Useful variables include `CC`, `CXX`, `PYTHON`, `EXTRA_CFLAGS`, `EXTRA_CXXFLAGS` and `EXTRA_LDFLAGS`.
2. Run make.
