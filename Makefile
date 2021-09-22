SRCDIR=src
IDIR=include
ODIR=bin
CXX=g++
CXXFLAGS=-c --std=gnu++17 -Wall -Wextra -I$(IDIR)
AR=ar
ARFLAGS=rsuU

EXENAME=libAppleLED
OBJ= $(patsubst $(SRCDIR)/%.cpp, $(ODIR)/%.o, $(wildcard $(SRCDIR)/*.cpp))
DEPS= $(wildcard $(IDIR)/*.h)

DEBUG ?= 0

ifeq ($(DEBUG), 1)
	CXXFLAGS += -ggdb3
	ODIR := $(ODIR)/debug
else
	CXXFLAGS += -O2
	ODIR := $(ODIR)/release
endif

all: $(ODIR) $(ODIR)/$(EXENAME).a

$(ODIR):
	$(shell mkdir -p $(ODIR))

$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CXX) -o $@ $< $(CXXFLAGS)

$(ODIR)/$(EXENAME).a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

.PHONY: clean $(ODIR)

clean:
	rm -rf $(firstword $(subst /,  , $(ODIR)))
