SRCDIR = .
INCDIR = ./include
DEPDIR = ../bench/deps/arch_dep/include
TARGET = persist

OBJS := $(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp))
SRCS := $(wildcard $(SRCDIR)/*.cpp)
INCS := $(wildcard $(DEPDIR)/*.h)
INCS += $(wildcard $(INCDIR)/*.h)

.PHONY: all clean


ifeq ($(CFG),debug)
  CFLAGS += -O0 -ggdb3 -DPDEBUG
else
  CFLAGS += -O2 -g
  CFLAGS += -march=native
endif

CC       := gcc
CFLAGS   += -std=c++17 -fno-strict-aliasing -fno-stack-protector -Wall -Wno-unused-function -Wno-unused-label
CFLAGS   += -I$(INCDIR) -I$(DEPDIR)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
AR       := ar rcs
LIBS     := -lpthread -lfolly


all:		lib$(TARGET).a

%.o:		%.c	$(INCS)
	$(CPP) $(CPPFLAGS) -c $< -o $@ 

lib$(TARGET).a: $(TARGET).o $(OBJS)
	$(AR) $@ $^

clean:
	rm *.a *.o