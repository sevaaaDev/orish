#ifndef _PARSER_H
#define _PARSER_H

#include "../arena.h"
#include "lexer.h"

enum Node_Kind {
    NODE_program,
    NODE_list,
    NODE_cmd,
    NODE_cmd_word,
};

typedef struct Node_da {
    size_t capacity;
    size_t count;
    struct Node **items;
} Node_da;

struct Parser {
    struct Lexer *l;
    struct Token *lookahead;
    struct Token *lookahead_2;
    jmp_buf *exception_jump;
    Arena *array_arena;
    Arena *main_arena;
};

struct Node {
    enum Node_Kind type;
    union {
        struct {
            char *cmd;
            char **argv;
        } simple;
    };
};

struct Parser parser_new(struct Lexer *l, jmp_buf *err_jmp, Arena *array_arena, Arena *main_arena);
struct Node * parse(struct Parser *p); 

#endif
