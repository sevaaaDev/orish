#include <string.h>
#include "../src/lexer.h"


#define test(expr, msg) \
    printf("FAIL "msg"\n"); \
    assert(expr); \
    printf("\033[1A\033[2KOK "msg"\n")

#define str_eq(a, b) !strcmp(a, b)

int
main()
{
    Arena a = {0};
    {
        struct Lexer lex = lexer_new("echo hello;");
        struct Token *tok = lexer_scan(&a, &lex);
        test(tok->type == 2, "TOKEN_word");
        tok = lexer_scan(&a, &lex);
        test(tok->type == 2, "TOKEN_word");
        tok = lexer_scan(&a, &lex);
        test(tok->type == 0, "TOKEN_separator");
    }
    {
        struct Lexer lex = lexer_new("echo 'hello world';");
        struct Token *tok = lexer_scan(&a, &lex);
        test(tok->type == 2, "echo");
        tok = lexer_scan(&a, &lex);
        test(tok->type == 2, "'hello world'");
        tok = lexer_scan(&a, &lex);
        test(tok->type == 0, ";");
    }
    {
        struct Lexer lex = lexer_new("'hello; world'");
        struct Token *tok = lexer_scan(&a, &lex);
        test(tok->type == 2, "'hello; world'");
        tok = lexer_scan(&a, &lex);
        test(!tok, "no token after quotes");
    }
    {
        struct Lexer lex = lexer_new("hello\\ world");
        struct Token *tok = lexer_scan(&a, &lex);
        test(str_eq("hello\\ world", tok->value), "'hello world' using backslash");
        tok = lexer_scan(&a, &lex);
        test(!tok, "no token after quotes");
    }
    arena_free(&a);
}

#define ARENA_IMPLEMENTATION
#include "../arena.h"
