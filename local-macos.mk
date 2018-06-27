INCLUDE_DIRS	= /sw/opt/boost-1_67/include
LIB_DIRS		= /sw/opt/boost-1_67/lib

CC				= clang-5.0
CXX				= clang++-5.0
GENGETOPT		= gengetopt
PYTHON			= python3.6

EXTRA_CXXFLAGS	= -mmacosx-version-min=10.11 -stdlib=libc++ -nostdinc++ -I/sw/include/c++/v1
EXTRA_LDFLAGS	= -mmacosx-version-min=10.11 -stdlib=libc++ -L/sw/lib -Wl,-rpath,/sw/lib
EXTRA_CFLAGS	= -mmacosx-version-min=10.11
