PROG := hashmap

SRCS += \
	hashmap.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/hashtable.c \
	$(LIB)/list.c \
	$(LIB)/pair.c \

#
OBJS := ${SRCS:.c=.o}
