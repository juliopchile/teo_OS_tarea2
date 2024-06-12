CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -g -D_XOPEN_SOURCE=500 -D_SVID_SOURCE -D_DEFAULT_SOURCE
LDLIBS := -lrt -lncurses -lpthread

SRC := main.c funciones.c
OBJ := $(SRC:.c=.o)
EXEC := main

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJ)
