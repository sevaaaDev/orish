CFLAGS = -Wall -Wextra -std=c23 -pedantic

ssshell: main.c
	$(CC) $(CFLAGS) main.c -o ssshell

debug: main.c
	$(CC) $(CFLAGS) -g main.c -o debug
