INCLUDE_DIRS		= /sw/opt/boost-1_67/include
LIB_DIRS			= /sw/opt/boost-1_67/lib

CC					= clang-6.0
CXX					= clang++-6.0
#CC = gcc-7
#CXX = g++-7
PYTHON				= python3.6

WARNING_FLAGS		= -Wall -Werror -Wno-unused-const-variable -Wno-unused-function
OPT_FLAGS			= -O0 -g

SYSTEM_CFLAGS		= -mmacosx-version-min=10.11
SYSTEM_CXXFLAGS		= -mmacosx-version-min=10.11 -stdlib=libc++ -nostdinc++ -I/sw/include/c++/v1
SYSTEM_LDFLAGS		= -mmacosx-version-min=10.11 -stdlib=libc++ -L/sw/lib -Wl,-rpath,/sw/lib

BOOST_ROOT			= /sw/opt/boost-1_67
BOOST_INCLUDE		= -I$(BOOST_ROOT)/include
BOOST_LIBS			= -L$(BOOST_ROOT)/lib -lboost_container-mt -lboost_iostreams-mt -lboost_serialization-mt -lboost_system-mt
LIBDISPATCH_LIBS	= 
