SRC_DIR=.
SRCS=$(SRC_DIR)/test_tb2uv.c \
     $(SRC_DIR)/tu_list.c \
     $(SRC_DIR)/jsw_rbtree.c \
     $(SRC_DIR)/tb2uv.c
     
OBJS=$(notdir $(SRCS:.c=.o))
TARGET=$(notdir $(SRCS:.c=))

MENU_OBJS=test_menu.o tu_list.o jsw_rbtree.o tb2uv.o cJSON.o
MENU_JSON_OBJS=test_menu_json.o tu_menu_json.o cJSON.o tu_list.o jsw_rbtree.o tb2uv.o
MENUBAR_JSON_OBJS=test_menubar_json.o tu_menubar_json.o cJSON.o tu_list.o jsw_rbtree.o tb2uv.o

#LIBUV_INC=/usr/include/uv
LIBUV_LDFLAGS=-L$(LIBUV_LIB) -luv

DFLAGS= -I. -I/usr/include
CFLAGS=-g
OPTIONS = 
LDFLAGS = -L/usr/lib/x86_64-linux-gnu -L/usr/lib -lpthread -lm -lrt -luv

CC=gcc
DEPENDFLAGS=-D__UNIX__ -D__LINUX__ 

DEFINES  = $(CFLAGS) $(OPTIONS) $(DFLAGS) $(LDFLAGS) $(DEPENDFLAGS)

all: $(TARGET) test_menu test_menu_json test_menubar_json

%.o: $(SRC_DIR)/%.c
	$(CC) $(INCLUDES) $(DEFINES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(DEFINES)

test_menu: $(MENU_OBJS)
	$(CC) -o $@ $^ $(DEFINES)

test_menu_json: $(MENU_JSON_OBJS)
	$(CC) -o $@ $^ $(DEFINES)

test_menubar_json: $(MENUBAR_JSON_OBJS)
	$(CC) -o $@ $^ $(DEFINES)

clean:
	rm -f *.o  $(TARGET) test_menu test_menu_json test_menubar_json core a.out $(OBJS)

depend:
	makedepend -- $(DEPENDFLAGS) $(CFLAGS) -- $(SRCS) 
# DO NOT DELETE
