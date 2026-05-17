#include <criterion/criterion.h>
#include <setjmp.h>

#include "../src/parser.h"
#include "../src/lexer.h"

#define ARENA_IMPLEMENTATION
#include "../arena.h"

Arena a = {0};
Arena m = {0};

Test(parser, empty) {
    jmp_buf j;
    if (setjmp(j) == 0) {
        struct Lexer l = lexer_new(""); 
        struct Parser p = parser_new(&l, &j, &a, &m);
        struct Node *node = parse(&p);
    } else {
        cr_assert(true);
        return;
    };
    cr_assert(false);
}

Test(parser, simple) {
    struct Lexer l = lexer_new("echo hello world"); 
    struct Parser p = parser_new(&l, NULL, &a, &m);
    struct Node *node = parse(&p);
    cr_assert(node->type == NODE_cmd);
    cr_assert(strcmp(node->simple.cmd, "echo") == 0);
    cr_assert(strcmp(node->simple.argv[1], "hello") == 0);
    cr_assert(strcmp(node->simple.argv[2], "world") == 0);
    cr_assert(node->simple.argv[3] == NULL);
}

Test(parser, simple_quoted) {
    struct Lexer l = lexer_new("echo 'hello world'"); 
    struct Parser p = parser_new(&l, NULL, &a, &m);
    struct Node *node = parse(&p);
    cr_assert(node->type == NODE_cmd);
    cr_assert(strcmp(node->simple.cmd, "echo") == 0);
    cr_assert(strcmp(node->simple.argv[1], "'hello world'") == 0);
    cr_assert(node->simple.argv[2] == NULL);
}
