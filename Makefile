CFLAGS = -Wall -Wextra -std=c23 -pedantic

orish: main.c
	$(CC) $(CFLAGS) main.c -o orish

debug: main.c
	$(CC) $(CFLAGS) -g main.c -o debug
