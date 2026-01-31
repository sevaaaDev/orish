#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void 
da_free(void** arr, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        free(arr[i]);
    }
    arrfree(arr);
}

enum TOKEN_TYPE {
    TOKEN_TYPE_word,
    TOKEN_TYPE_separator
};

typedef struct {
    enum TOKEN_TYPE type; 
    void* value;
} Token;

void 
lexer_test(Token** tokens) {
    for (int i = 0; tokens[i]; ++i) {
        Token* t = tokens[i];
        if (t->value)
            printf("type: %d, value: %s, len: %ld\n", t->type, (char*)t->value, strlen((char*)t->value));
        else
            printf("type: %d\n", t->type);
    } 
}

Token**
addToken(Token** token_arr, enum TOKEN_TYPE t, void* value) {
    Token* tok = malloc(sizeof(Token));
    assert(tok);
    tok->type = t;
    tok->value = value;
    arrput(token_arr, tok);
    return token_arr;
}

Token**
lexer(char* line) {
    Token** token_arr = NULL;
    char* cur = line;
    while (*cur != '\0') {
        char* start = cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            token_arr = addToken(token_arr, TOKEN_TYPE_separator, NULL);
            break;
        default: 
            while (*(cur) != ';' && *(cur) != '\0' && *(cur) != ' ') cur++;
            int len = cur - start;  
            char* cmd = strndup(start, len); 

            token_arr = addToken(token_arr, TOKEN_TYPE_word, cmd);
        }
    }
    // TODO: should you do arrput(token_arr, NULL)?
    return token_arr;
}

enum TREE_NODE_TYPE {
    TREE_NODE_TYPE_list,
    TREE_NODE_TYPE_cmd,
    TREE_NODE_TYPE_cmd_word,
};

typedef struct Node {
    enum TREE_NODE_TYPE type;
    Token* token;
    struct Node** children;
} Node;

Node*
make_node(enum TREE_NODE_TYPE t, Token* tok) {
    Node* node = malloc(sizeof(Node));
    node->type = t; 
    node->token = tok; 
    node->children = NULL;
    return node;
}

typedef struct TokenIter {
    size_t i;
    size_t len;
    Token** buf;
} TokenIter;

TokenIter
make_token_iter(Token** tok) {
    TokenIter iter = {.i = 0, .buf = tok};
    iter.len = arrlen(tok);
    return iter;
}

bool
tok_match(TokenIter* iter, enum TOKEN_TYPE type) {
    return (iter->i < iter->len && iter->buf[iter->i]->type == type);
}

Token*
tok_consume(TokenIter* iter) {
    return iter->buf[iter->i++];
}

Token*
tok_peek(TokenIter* iter) {
    return iter->buf[iter->i];
}

enum PARSER_ERROR_TYPE {
    PARSER_ERROR_TYPE_success,
    PARSER_ERROR_TYPE_unexpected_token,
};

char* PARSER_ERROR_TABLE[] = {
    [PARSER_ERROR_TYPE_success] = "Success",
    [PARSER_ERROR_TYPE_unexpected_token] = "Unexpected token",
};

typedef struct ParserError {
    enum PARSER_ERROR_TYPE type;
    Token* token;
} ParserError;

ParserError
parse_simple_command(TokenIter* iter, Node** out) {
    if (!tok_match(iter, TOKEN_TYPE_word)) { 
        return (ParserError){.type = PARSER_ERROR_TYPE_unexpected_token, .token = tok_consume(iter) };
    }
    Node *node = make_node(TREE_NODE_TYPE_cmd, NULL);
    do {
        Token* tok = tok_consume(iter);
        arrput(node->children, make_node(TREE_NODE_TYPE_cmd_word, tok));
    } while(tok_match(iter, TOKEN_TYPE_word));
    *out = node;
    return (ParserError){.type = PARSER_ERROR_TYPE_success, .token = NULL };
}

int
main(int argc, char** argv) {
    if (argc == 1) return 1; 
    char* commands = argv[1];

    Token** tokens = lexer(commands);
    if (!tokens) return 2;
    TokenIter iter = make_token_iter(tokens);
    Node* root;
    ParserError err = parse_simple_command(&iter, &root);
    printf("%s\n", PARSER_ERROR_TABLE[err.type]);
}
