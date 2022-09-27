CC=gcc
INC_DIR = ./include
SRC_DIR = ./source
CFLAGS=-pthread -Wall -Wextra -std=c99 -D _DEFAULT_SOURCE -I$(INC_DIR)

cut: $(SRC_DIR)/cut.c
	$(CC) $(CFLAGS) $(SRC_DIR)/cut.c -o cut 