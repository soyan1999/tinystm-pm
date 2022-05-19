PROG := rbtree_

SRCS += \
	rbtree_.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/rbtree.c \
	$(LIB)/memory.c \

#
OBJS := ${SRCS:.c=.o}
