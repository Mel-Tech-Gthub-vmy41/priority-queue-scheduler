CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
TARGET  = scheduler

all: $(TARGET)

$(TARGET): scheduler.c
	$(CC) $(CFLAGS) -o $(TARGET) scheduler.c

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run

