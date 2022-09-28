CC=gcc
INC_DIR = ./include
SRC_DIR = ./source
OBJ_DIR = ./obj
CFLAGS=-pthread -Wall -Wextra -std=c99 -D _DEFAULT_SOURCE -I$(INC_DIR)

CFILES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CFILES))

TARGET= cut

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^

$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

clean: 
	rm -rf $(TARGET) $(OBJ_DIR)/