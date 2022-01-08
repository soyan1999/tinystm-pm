SRCDIR = .
INCDIR = ./include
TARGET = persist

OBJS := $(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp))
SRCS := $(wildcard $(SRCDIR)/*.cpp)
INCS := $(wildcard $(SRCDIR)/*.h)
INCS += $(wildcard $(INCDIR)/*.h)

.PHONY: all clean


ifeq ($(CFG),debug)
  CFLAGS += -O0 -ggdb3 -DPDEBUG
else
  CFLAGS += -O2 -g
  CFLAGS += -march=native
endif

CC       := gcc
CFLAGS   += -std=c++11 -fno-strict-aliasing -fno-stack-protector -Wall -Wno-unused-function -Wno-unused-label
CFLAGS   += -I$(INCDIR) -I$(SRCDIR)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
AR       := ar rcs
LIBS     := -lpthread


all:		lib$(TARGET).a

%.o:		%.c	$(INCS)
	$(CPP) $(CPPFLAGS) -I$(SRCDIR) -I$(INCDIR) -c $< -o $@ 

lib$(TARGET).a: $(TARGET).o $(OBJS)
	$(AR) $@ $^

clean:
	rm *.a *.o