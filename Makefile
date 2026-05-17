CFLAGS = -Wall -Wextra -std=c23 -pedantic

orish: ./src/main.c ./src/lexer.c ./src/lexer.h builtin.h arena.h
	$(CC) $(CFLAGS) ./src/main.c ./src/lexer.c ./src/parser.c -o orish

debug: ./src/main.c ./src/lexer.c ./src/lexer.h builtin.h arena.h
	$(CC) $(CFLAGS) -g ./src/main.c ./src/lexer.c ./src/parser.c -o debug

clean:
	rm -f orish debug
