INCLUDE_DIRS	?=
LIB_DIRS		?=
LIBRARIES		?=

GENGETOPT		?= gengetopt
PYTHON			?= python
PG_CONFIG		?= pg_config
GRUNT			?= grunt
YARN			?= yarn
CP				?= cp

WARNING_FLAGS	?=
OPT_FLAGS		?=

PYTHON_CFLAGS	?=
PYTHON_CXXFLAGS	?=
PYTHON_LDFLAGS	?=

BOOST_LIBRARY_SUFFIX	?=
BOOST_ROOT		?= /usr
BOOST_INCLUDE	= $(BOOST_ROOT)/include
BOOST_LIB		= $(BOOST_ROOT)/lib
LIB_DIRS		+= $(BOOST_LIB)

INCLUDE_DIRS	+= ../include ../lib/libbio/include ../lib/libbio/lib/range-v3/include ../lib/libbio/lib/GSL/include $(BOOST_INCLUDE)
LIBRARIES		+= boost_system$(BOOST_LIBRARY_SUFFIX)

CPPFLAGS		+= $(patsubst %,-I%,$(strip $(INCLUDE_DIRS)))
CFLAGS			+= $(WARNING_FLAGS) $(OPT_FLAGS) $(SYSTEM_CFLAGS) -std=c99
CXXFLAGS		+= $(WARNING_FLAGS) $(OPT_FLAGS) $(SYSTEM_CXXFLAGS) -std=c++17
LDFLAGS			+= $(WARNING_FLAGS) $(OPT_FLAGS) $(SYSTEM_LDFLAGS) -std=c++17 $(patsubst %,-L%,$(strip $(LIB_DIRS))) $(patsubst %,-l%,$(strip $(LIBRARIES)))

PYTHON_CFLAGS	+= $(SYSTEM_CFLAGS) -std=c++17
PYTHON_CXXFLAGS	+= $(SYSTEM_CXXFLAGS) -std=c++17
PYTHON_LDFLAGS	+= $(SYSTEM_LDFLAGS) -std=c99
