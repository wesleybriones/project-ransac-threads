CC = gcc
CFLAGS = -Wall -Wextra

all: ransac

ransac: main.c
	$(CC) $(CFLAGS) -o ransac main.c

clean:
	rm -f ransac
