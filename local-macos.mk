CLANG_ROOT			= /usr/local/homebrew/opt/llvm

INCLUDE_DIRS		= /usr/local/homebrew/opt/boost/include
LIB_DIRS			= /usr/local/homebrew/opt/boost/lib

CC					= /usr/local/homebrew/opt/llvm/bin/clang
CXX					= /usr/local/homebrew/opt/llvm/bin/clang++
#CC = gcc-7
#CXX = g++-7
PYTHON				= python3.6m

WARNING_FLAGS		= -Wall -Werror -Wno-unused-const-variable -Wno-unused-function
OPT_FLAGS			= -O2 -g

CPPFLAGS			= -D_LIBCPP_DISABLE_AVAILABILITY
SYSTEM_CFLAGS		= -mmacosx-version-min=10.13
SYSTEM_CXXFLAGS		= -mmacosx-version-min=10.13 -faligned-allocation -stdlib=libc++ -nostdinc++ -I$(CLANG_ROOT)/include/c++/v1
SYSTEM_LDFLAGS		= -mmacosx-version-min=10.13 -faligned-allocation -stdlib=libc++ -L$(CLANG_ROOT)/lib -Wl,-rpath,$(CLANG_ROOT)/lib

BOOST_ROOT			= /usr/local/homebrew/opt/boost
BOOST_INCLUDE		= -I$(BOOST_ROOT)/include
BOOST_LIBS			= -L$(BOOST_ROOT)/lib -lboost_container-mt -lboost_iostreams-mt -lboost_serialization-mt -lboost_system-mt
