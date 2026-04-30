SRC_DIR=.
SRCS=$(SRC_DIR)/main_tb2uv.c \
     $(SRC_DIR)/tu_list.c \
     $(SRC_DIR)/jsw_rbtree.c \
     $(SRC_DIR)/cJSON.c \
     $(SRC_DIR)/tb2uv.c
     
OBJS=$(notdir $(SRCS:.c=.o))
TARGET=$(notdir $(SRCS:.c=))


#LIBUV_INC=/usr/include/uv
LIBUV_LDFLAGS=-L$(LIBUV_LIB) -luv

DFLAGS= -I. -I/usr/include
CFLAGS=-g
OPTIONS = 
LDFLAGS = -L/usr/lib/x86_64-linux-gnu -L/usr/lib -lpthread -lm -lrt -lsqlite3 -luv

CC=gcc
BIN=Linux_3.10.0-1062.el7.x86_64
INSTALL_DIR=../lib
SOURCE_DIR=.
DEPENDFLAGS=-D__UNIX__ -D__LINUX__ 


DEFINES  = $(CFLAGS) $(OPTIONS) $(DFLAGS) $(LDFLAGS) $(DEPENDFLAGS)


all: $(TARGET)


%.o: $(SRC_DIR)/%.c
	$(CC) $(INCLUDES) $(DEFINES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(DEFINES)
	#cp $(TARGET) $(BOS_EXE)

clean:
	rm -f *.o  $(TARGET) core a.out $(OBJS)

depend:
	makedepend -- $(DEPENDFLAGS) $(CFLAGS) -- $(SRCS) 
# DO NOT DELETE

