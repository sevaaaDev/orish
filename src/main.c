#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
// TODO: split code to different files
// TODO: add testing

#include "./lexer.h"


/* ===== parser ===== */
enum Ast_Type {
    AST_TYPE_program,
    AST_TYPE_list,
    AST_TYPE_cmd,
    AST_TYPE_cmd_word,
};

struct Ast_Node_da {
    size_t capacity;
    size_t count;
    struct Ast_Node **items;
};

struct Ast_Node {
    enum Ast_Type type;
    const struct Token *token;
    struct Ast_Node_da children;
};

struct Ast_Node *
make_node(Arena *arena, enum Ast_Type t, struct Token *tok) {
    struct Ast_Node *node = arena_alloc(arena, sizeof(struct Ast_Node));
    node->type = t; 
    node->token = tok; 
    node->children = (struct Ast_Node_da){0};
    return node;
}

enum Parser_Stat_Kind {
    PARSER_STAT_KIND_unexpected_token,
    PARSER_STAT_KIND_missing_token,
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
    Parser_Error err;
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
    struct Ast_Node *node = make_node(arena, AST_TYPE_list, NULL);
    struct Ast_Node *cmd; 
    do {
        Parser_Error *err = parse_simple_command(arena, p, &cmd);
        if (err) return err;
        arena_da_append(arena, &node->children, cmd);
        if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_separator)
            consume_token(arena, p);
        *out = node;
    } while(peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_word);
    return NULL;
}

Parser_Error *
parse_complete_cmd(Arena *arena, struct Parser *p, struct Ast_Node **out) {
    *out = NULL;
    struct Ast_Node *list; 
    Parser_Error *err = parse_list(arena, p, &list);
    if (err) return err;
    if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_separator) {
        consume_token(arena, p);
    }
    *out = list;
    return NULL;
}

Parser_Error *
parse_all_commands(Arena *arena, struct Parser *p, struct Ast_Node **out) {
    *out = NULL;
    struct Ast_Node *all_cmds = make_node(arena, AST_TYPE_program, NULL);;
    if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_linebreak)
        consume_token(arena, p);
    while (peek_token(arena, p)) {
        struct Ast_Node *list;
        Parser_Error *err = parse_complete_cmd(arena, p, &list);
        if (err) return err;
        arena_da_append(arena, &all_cmds->children, list);
        if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_linebreak)
            consume_token(arena, p);
    }
    *out = all_cmds;
    return NULL;
}

/* ===== exec ===== */
#include "../builtin.h"

struct Runtime_Error {
    int exit_code;
    char *msg;
};

// TODO: pass context to exec()
typedef struct Context {
    int exit_code;
    char *prog_name;
} Context;

void
exec_cmd(struct Ast_Node *root, Context *ctx) {
    if (root->type != AST_TYPE_cmd) return;
    /* WARN: is empty init equal to zero init? the last elem must be 0 */
    /* WARN: variable size array means we cant do argument expansion,
     *       because the argc will be greater than children count */
    char *argv[root->children.count+1] = {};
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        argv[i] = child->token->value;
    }
    if (!strcmp(argv[0], "exit")) {
        ctx->exit_code = orish_builtin_exit(root->children.count, argv);
        return;
    }
    if (!strcmp(argv[0], "cd")) {
        ctx->exit_code = orish_builtin_cd(root->children.count, argv, ctx->prog_name);
        return;
    }
    pid_t pid = fork();
    if (pid == -1) return; 
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) {
            perror(argv[0]);
            exit(127);
        } 
    }
    return;
}

void
exec_list(struct Ast_Node *root, Context *ctx) {
    if (root->type != AST_TYPE_list) return;
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        switch (child->type) {
            case AST_TYPE_cmd:
                exec_cmd(child, ctx);
                wait(NULL);
                break;
            default:
                assert(0 == "TODO: implement exec non simple cmd");
        }
    }
}

void
exec_prog(struct Ast_Node *root, Context *ctx) {
    if (root->type != AST_TYPE_program) return;
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        if (child->type != AST_TYPE_list) return;
        exec_list(child, ctx);
    }
}

/* ===== orish ===== */
enum Error_Kind {
    Success,
    ERROR_parser_err,
    ERROR_runtime_err,
};

// TODO: remove this bcs not needed
typedef struct Error {
    enum Error_Kind kind;
    union {
        Parser_Error *parser;
    } data;
} Error;  

Error
orish_eval(Arena *arena, const char *input, Context *ctx) {
    struct Lexer lexer = lexer_new(input);
    struct Parser parser = parser_new(&lexer);
    struct Ast_Node *root;
    Parser_Error *err = parse_all_commands(arena, &parser, &root);
    if (err) {
        return (Error){
            .kind = ERROR_parser_err,
            .data.parser = err,
        };
    }
    exec_prog(root, ctx);
    return (Error){
        .kind = Success,
    };
}

#define GGETS_IMPLEMENTATION
#include "../ggets.h"

struct Flags {
    bool interactive;
    char *filename;
    char *cmd_string;
};

int
main(int argc, char **argv) {
    int ret = 0;
    struct Flags flags = {0};
    flags.interactive = true;
    if (argc > 1) {
        flags.interactive = false;
        if (strcmp(argv[1], "-c") == 0) {
            if (argc == 2) return 1;
            flags.cmd_string = argv[2];
        } else {
            flags.filename = argv[1];
        }
    }
    Context ctx = {.prog_name = argv[0]};
    Arena main_arena = {0};
    // TODO: simplify branch
    if (flags.cmd_string) {
        char *commands = flags.cmd_string;
        Error err = orish_eval(&main_arena, commands, &ctx);
        if (err.kind == ERROR_parser_err) {
            ret = 2;
            printf("%s: expecting %s, got '%s'\n", argv[0], err.data.parser->expect, err.data.parser->got);
            goto quit_no_cleanup;
        }
    }
    if (flags.filename) {
        int fd = open(flags.filename, O_RDONLY);
        if (fd <= -1) {
            printf("%s: internal error: %s\n", argv[0], strerror(errno));
            ret = 1;
            goto quit_no_cleanup;
        }
        struct stat stat;
        fstat(fd, &stat);
        char *commands = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (commands == MAP_FAILED) {
            printf("%s: internal error: %s\n", argv[0], strerror(errno));
            ret = 1;
            goto quit_no_cleanup;
        }
        Error err = orish_eval(&main_arena, commands, &ctx);
        if (munmap(commands, stat.st_size) <= -1) {
            printf("%s: internal error %s\n", argv[0], strerror(errno));
        }
        if (err.kind) {
            ret = 2;
            if (err.kind == ERROR_parser_err) {
                printf("%s: expecting %s, got '%s'\n", argv[0], err.data.parser->expect, err.data.parser->got);
            }
            goto cleanup;
        }
    }
    bool running = true;
    while (flags.interactive && running) {
        printf("orish> ");
        char *commands = NULL;
        int ggets_err = ggets(&commands);
        if (ggets_err == EOF) {
            printf("\nEOF\n");
            running = false;
            continue;
        };
        if (ggets_err) exit(34);
        Error err = orish_eval(&main_arena, commands, &ctx);
        free(commands);
        if (err.kind == ERROR_runtime_err) {
            ret = 2;
            goto cleanup;
        }
        if (err.kind == ERROR_parser_err) {
            printf("%s: expecting %s, got '%s'\n", argv[0], err.data.parser->expect, err.data.parser->got);
        }
    }
    cleanup:
        arena_free(&main_arena);
    quit_no_cleanup:
    return ret;
}

#define ARENA_IMPLEMENTATION
#include "../arena.h"

