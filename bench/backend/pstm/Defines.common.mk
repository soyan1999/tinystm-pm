LIB      := ../lib

CC       := gcc
# CFLAGS   += -std=c++11 -w -fpermissive -mrtm -O3
CFLAGS   += -std=c++11 -fno-strict-aliasing -fno-stack-protector -Wall -Wno-unused-function -Wno-unused-label -g -O3
CFLAGS   += -I $(LIB)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     := -lpthread

# Remove these files when doing clean
OUTPUT +=
