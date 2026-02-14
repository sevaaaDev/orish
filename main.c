#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#define ARENA_IMPLEMENTATION
#include "arena.h"

/* ===== lexer ===== */
struct Lexer {
    const char *buf_start;
    const char *cur;
    const char *last_newline;
    size_t cur_line;
};

struct Lexer 
lexer_new(const char *input) {
    struct Lexer l;
    l.buf_start = input;
    l.cur = input;
    l.last_newline = input;
    l.cur_line = 1;
    return l;
}

/* these are generic token, you must use TODO: lexer_classify_reserved() to determine the specific token.
 * all reserved word is also a normal word, this mean you can use TOKEN_reserved
 * in the place of normal word.
 * 
 * TOKEN_reserved can only be converted to specific reserved word if and only if it was preceeded by
 * TOKEN_separator or linebreak, else it is treated as normal word.
 * */
enum Token_Type {
    TOKEN_separator,
    TOKEN_word,
    TOKEN_reserved,
};

struct Token {
    enum Token_Type type; 
    char *value;
}; 

struct Token *
make_token(Arena *arena, enum Token_Type t, char *value) {
    struct Token *tok = arena_alloc(arena, sizeof(struct Token));
    if (!tok) {
        printf("arena_alloc error");
        exit(1);
    }
    tok->type = t;
    tok->value = value;
    return tok;
}

char *arena_strndup(Arena *arena, const char *src, size_t len) {
    char *str = arena_alloc(arena, len+1);
    strncpy(str, src, len);
    str[len] = '\0';
    return str;
}

/* handle reading from file (handle newline) */
struct Token *
lexer_scan(Arena *arena, struct Lexer *l) {
    while (*(l->cur) != '\0') {
        const char *start = l->cur++;
        switch (*start) {
        case ' ':
        case '\n':
            break;
        case ';':
            return make_token(arena, TOKEN_separator, arena_strndup(arena, start, l->cur - start));
            break;
        default: 
            while (*(l->cur) != ';' 
                && *(l->cur) != '\0' 
                && *(l->cur) != ' ' 
                && *(l->cur) != '\n') l->cur++;
            int len = l->cur - start;  
            char *cmd = arena_strndup(arena, start, len);
            return make_token(arena, TOKEN_word, cmd);
        }
    }
    return NULL;
}

/* this function will be used when we implement keyword */
/* TODO: find good name for this
 *    - lexer_token_classified_as
 *    - lexer_token_allow_reclassify
 * */
bool
lexer_classify_word(enum Token_Type type, const struct Token *tok) {
    if (tok->type != TOKEN_word) return false;
    switch (type) {
    case TOKEN_word:
        return true;
    default:
        return false;
    }
}

/* ===== parser ===== */
enum Ast_Type {
    AST_TYPE_list,
    AST_TYPE_cmd,
    AST_TYPE_cmd_word,
};

struct Array_Ast_Node {
    size_t capacity;
    size_t count;
    struct Ast_Node **items;
};

struct Ast_Node {
    enum Ast_Type type;
    const struct Token *token;
    struct Array_Ast_Node children;
};

struct Ast_Node *
make_node(Arena *arena, enum Ast_Type t, struct Token *tok) {
    struct Ast_Node *node = arena_alloc(arena, sizeof(struct Ast_Node));
    node->type = t; 
    node->token = tok; 
    node->children = (struct Array_Ast_Node){0};
    return node;
}

enum Parser_Stat_Kind {
    PARSER_STAT_KIND_success,
    PARSER_STAT_KIND_unexpected_token,
    PARSER_STAT_KIND_missing_token,
};

static const char *PARSER_ERROR_TABLE[] = {
    [PARSER_STAT_KIND_unexpected_token] = "Unexpected token: %s",
};

typedef struct Parser_Error {
    enum Parser_Stat_Kind kind;
    struct Token *tok_gotten;
    const char *got;
    const char *expect;
} Parser_Error;

Parser_Error *
make_parser_error(Arena *arena, struct Token *tok, const char *got, const char* expect) {
    Parser_Error *err = arena_alloc(arena, sizeof(Parser_Error));
    err->tok_gotten = tok;
    err->got = got;
    err->expect = expect;
    err->kind = 0;
    return err;
}

struct Parser {
    struct Lexer *l;
    struct Token *lookahead;
    struct Token *lookahead_2;
};

struct Parser 
parser_new(struct Lexer *l) {
    struct Parser p;
    p.l = l;
    p.lookahead = NULL;
    p.lookahead_2 = NULL;
    return p;
}

const struct Token *
peek_token(Arena *arena, struct Parser *p) {
    if (p->lookahead == NULL)
        p->lookahead = lexer_scan(arena, p->l);
    return p->lookahead;
}
const struct Token *
peek_token_2(Arena *arena, struct Parser *p) {
    if (p->lookahead_2 == NULL)
        p->lookahead_2 = lexer_scan(arena, p->l);
    return p->lookahead_2;
}

struct Token *
consume_token(Arena *arena, struct Parser *p) {
    if (!p->lookahead) { 
        assert(!p->lookahead_2);
        p->lookahead = lexer_scan(arena, p->l);
    }
    struct Token *tok = p->lookahead;
    p->lookahead = p->lookahead_2;
    p->lookahead_2 = NULL;
    return tok;
}

/* TODO: overhaul error handling */
Parser_Error *
parse_simple_command(Arena *arena, struct Parser *p, struct Ast_Node **out) {
    if (!peek_token(arena, p)) {
       return make_parser_error(arena, consume_token(arena, p), "", "string");
    }
    if (!lexer_classify_word(TOKEN_word, peek_token(arena, p))) {
       struct Token *got = consume_token(arena, p);
       return make_parser_error(arena, got, got->value, "command");
    }
    struct Ast_Node *node = make_node(arena, AST_TYPE_cmd, NULL);
    do {
        struct Token *tok = consume_token(arena, p);
        struct Ast_Node *arg_node = make_node(arena, AST_TYPE_cmd_word, tok);
        arena_da_append(arena, &node->children, arg_node);
    } while(peek_token(arena, p) && lexer_classify_word(TOKEN_word, peek_token(arena, p)));
    *out = node;
    return NULL;
}

Parser_Error *
parse_list(Arena *arena, struct Parser *p, struct Ast_Node **out) {
    *out = NULL;
    struct Ast_Node *cmd; 
    Parser_Error *err = parse_simple_command(arena, p, &cmd);
    if (err) return err;
    struct Ast_Node *node = make_node(arena, AST_TYPE_list, NULL);
    arena_da_append(arena, &node->children, cmd);
    *out = node;
    while(peek_token(arena, p) && peek_token_2(arena, p)) {  
        if (peek_token(arena, p)->type != TOKEN_separator) break;
        if (peek_token_2(arena, p)->type == TOKEN_reserved) break;
        consume_token(arena, p);                                            // consume the semicolon
        Parser_Error *err = parse_simple_command(arena, p, &cmd);
        if (err) return err;
        arena_da_append(arena, &node->children, cmd);
    }
    return NULL;
}

Parser_Error *
parse_complete_cmd(Arena *arena, struct Parser *p, struct Ast_Node **out) {
    *out = NULL;
    struct Ast_Node *list; 
    Parser_Error *err = parse_list(arena, p, &list);
    if (err) return err;
    if (peek_token(arena, p) && peek_token(arena, p)->type != TOKEN_separator) {
       struct Token *got = consume_token(arena, p);
       return make_parser_error(arena, got, got->value, ";");
    }
    consume_token(arena, p);
    if (peek_token_2(arena, p)) {
       struct Token *got = consume_token(arena, p);
       return make_parser_error(arena, got, got->value, "");
    }
    *out = list;
    return NULL;
}

/* ===== exec ===== */
void
exec_cmd(struct Ast_Node *root) {
    if (root->type != AST_TYPE_cmd) return;
    /* WARN: is empty init equal to zero init? the last elem must be 0 */
    /* WARN: variable size array means we cant do argument expansion,
     *       because the argc will be greater than children count */
    char *argv[root->children.count+1] = {};
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        argv[i] = child->token->value;
    }
    pid_t pid = fork();
    if (pid == -1) return; 
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) {
            perror(argv[0]);
            exit(127);
        } 
    } else {
        wait(NULL);
    }
}

void
exec_prog(struct Ast_Node *root) {
    if (root->type != AST_TYPE_list) return;
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        switch (child->type) {
            case AST_TYPE_cmd:
                exec_cmd(child);
                break;
            default:
                assert(0 == "TODO: implement exec non simple cmd");
        }
    }
}


/* ===== orish ===== */
enum Error_Kind {
    Success,
    Parser_Err,
    Runtime_Err,
};

typedef struct Error {
    enum Error_Kind kind;
    union {
        Parser_Error *parser;
    } data;
} Error;  

Error
orish_eval(Arena *arena, const char *input) {
    struct Lexer lexer = lexer_new(input);
    struct Parser parser = parser_new(&lexer);
    struct Ast_Node *root;
    Parser_Error *err = parse_complete_cmd(arena, &parser, &root);
    if (err) {
        return (Error){
            .kind = Parser_Err,
            .data.parser = err,
        };
    }
    exec_prog(root);
    return (Error){
        .kind = Success,
    };
}

#define GGETS_IMPLEMENTATION
#include "ggets.h"

int
main(int argc, char **argv) {
    int ret = 0;
    bool interactive = false;
    if (argc == 1) {
        interactive = true; 
    }
    /* TODO: read input from file */
    Arena main_arena = {0};
    if (!interactive) {
        char *commands = argv[1];
        Error err = orish_eval(&main_arena, commands);
        if (err.kind) {
            ret = 3;
            goto quit;
        }

    }
    bool running = true;
    /* TODO: handle parser error */
    while (interactive && running) {
        printf("orish> ");
        char *commands = NULL;
        int ggets_err = ggets(&commands);
        if (ggets_err == EOF) {
            printf("\nEOF\n");
            running = false;
            continue;
        };
        if (ggets_err) exit(34);
        Error err = orish_eval(&main_arena, commands);
        free(commands);
        /* TODO: implement "exit" builtin command */
        if (err.kind == Runtime_Err) {
            ret = 3;
            goto quit;
        }
    }
    quit:
        arena_free(&main_arena);
    return ret;
}
