CC = g++
CFLAGS = -Wall -Wextra -Werror -pedantic

TARGET = polvo-cpp
SRC = src/*.cpp
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean
