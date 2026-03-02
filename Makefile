CFLAGS = -Wall -Wextra -std=c23 -pedantic

orish: ./src/main.c ./src/lexer.c ./src/lexer.h builtin.h arena.h
	$(CC) $(CFLAGS) ./src/main.c ./src/lexer.c -o orish

debug: main.c builtin.h arena.h
	$(CC) $(CFLAGS) -g ./src/main.c ./src/lexer.c -o debug

clean:
	rm -f orish debug
