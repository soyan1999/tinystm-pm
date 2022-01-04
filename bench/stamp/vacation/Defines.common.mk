CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DMAP_USE_RBTREE
# CFLAGS += -DMAP_USE_HASHTABLE

PROG := vacation

SRCS += \
	client.c \
	customer.c \
	manager.c \
	reservation.c \
	vacation.c \
	$(LIB)/list.c \
	$(LIB)/pair.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/rbtree.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
# TODO: Try a hashtable
# $(LIB)/hashtable.c

OBJS := ${SRCS:.c=.o}
