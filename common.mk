INCLUDE_DIRS	?=
LIB_DIRS		?=
LIBRARIES		?=

EXTRA_CXXFLAGS	?=
EXTRA_LDFLAGS	?=
EXTRA_CFLAGS	?=

CC				?= cc
CXX				?= c++
GENGETOPT		?= gengetopt
PYTHON			?= python
PG_CONFIG		?= pg_config

INCLUDE_DIRS	+= ../include
LIBRARIES		+= boost_system-mt

EXTRA_CXXFLAGS	+= -std=c++17
EXTRA_LDFLAGS	+= -std=c++17
EXTRA_CFLAGS	+= -std=c99

CPPFLAGS		= $(patsubst %,-I%,$(strip $(INCLUDE_DIRS)))
CFLAGS			= $(EXTRA_CFLAGS)
CXXFLAGS		= $(EXTRA_CXXFLAGS)
LDFLAGS			= $(EXTRA_LDFLAGS) $(patsubst %,-L%,$(strip $(LIB_DIRS))) $(patsubst %,-l%,$(strip $(LIBRARIES)))
