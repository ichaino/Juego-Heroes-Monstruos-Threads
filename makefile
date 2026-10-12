# Makefile para Tarea 2 - Sistemas Operativos (Simulación Doom)

CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = Juego_SO
SRC = Juego_SO.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) ejemplo1.txt

clean:
	rm -f $(TARGET)
