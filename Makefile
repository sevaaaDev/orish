CFLAGS = -Wall -Wextra -std=c23 -pedantic

orish: main.c builtin.h arena.h
	$(CC) $(CFLAGS) main.c -o orish

debug: main.c builtin.h arena.h
	$(CC) $(CFLAGS) -g main.c -o debug

clean:
	rm -f orish debug
