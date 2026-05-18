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
#include "./parser.h"

char *prog_name = "orish";

/* ===== exec ===== */
#include "../builtin.h"

// TODO: pass context to exec()
typedef struct Context {
    int exit_code;
    Arena *arena;
} Context;

int
exec_cmd(struct Node *root, Context *ctx) {
    if (root->type != NODE_cmd) return 1;
    /* WARN: variable size array means we cant do argument expansion,
     *       because the argc will be greater than children count */
    char **argv = root->simple.argv;
    for (char **cur = argv; *cur != NULL; cur++) {
        *cur = lexer_util_quote_remover(ctx->arena, *cur);
    }
    // if (!strcmp(argv[0], "exit")) {
    //     ctx->exit_code = orish_builtin_exit(root->children.count, argv);
    //     return;
    // }
    // if (!strcmp(argv[0], "cd")) {
    //     ctx->exit_code = orish_builtin_cd(root->children.count, argv, ctx->prog_name);
    //     return;
    // }
    pid_t pid = fork();
    if (pid == -1) return 1; 
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) {
            perror(argv[0]);
            exit(127);
        } 
    }
    wait(NULL);
    return 1;
}

// int
// exec_list(struct Ast_Node *root, Context *ctx) {
//     if (root->type != AST_TYPE_list) return;
//     for (size_t i = 0; i < root->children.count; ++i) {
//         struct Ast_Node *child = root->children.items[i];
//         switch (child->type) {
//             case AST_TYPE_cmd:
//                 exec_cmd(child, ctx);
//                 wait(NULL);
//                 break;
//             default:
//                 assert(0 == "TODO: implement exec non simple cmd");
//         }
//     }
// }

// int
// exec_prog(struct Ast_Node *root, Context *ctx) {
//     if (root->type != AST_TYPE_program) return;
//     for (size_t i = 0; i < root->children.count; ++i) {
//         struct Ast_Node *child = root->children.items[i];
//         if (child->type != AST_TYPE_list) return;
//         exec_list(child, ctx);
//     }
// }

/* ===== orish ===== */
enum Error_Kind {
    Success,
    ERROR_parser_err,
    ERROR_runtime_err,
};

// TODO: remove this bcs not needed
typedef struct Error {
    enum Error_Kind kind;
} Error;  

Error
orish_eval(Arena *arena, const char *input, Context *ctx) {
}

#define GGETS_IMPLEMENTATION
#include "../ggets.h"

struct Flags {
    bool interactive;
    char *filename;
    char *cmd_string;
};

jmp_buf main_jump;

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
    prog_name = argv[0];
    Arena main_arena = {0};
    Arena second_arena = {0};
    Context ctx = {.arena = &main_arena};
    // TODO: simplify branch
    if (flags.cmd_string) {
        char *commands = flags.cmd_string;
        Error err = orish_eval(&main_arena, commands, &ctx);
        if (err.kind == ERROR_parser_err) {
            ret = 2;
            // printf("%s: expecting %s, got '%s'\n", argv[0], err.data.parser->expect, err.data.parser->got);
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
                // printf("%s: expecting %s, got '%s'\n", argv[0], err.data.parser->expect, err.data.parser->got);
            }
            goto cleanup;
        }
    }
    bool running = true;
    while (flags.interactive && running) {
        char *commands = NULL;
        if (setjmp(main_jump) == 0) {
            printf("orish> ");
            int ggets_err = ggets(&commands);
            if (ggets_err == EOF) {
                printf("\nEOF\n");
                running = false;
                continue;
            };
            if (ggets_err) exit(34);
            struct Lexer lexer = lexer_new(commands);
            struct Parser parser = parser_new(&lexer, &main_jump, &second_arena, &main_arena);
            struct Node *root = parse(&parser);
            int status = exec_cmd(root, &ctx);
            // if (err.kind == ERROR_runtime_err) {
            //     ret = 2;
            //     goto cleanup;
            // }
            // if (err.kind == ERROR_parser_err) {
            //     printf("%s: expecting %s, got '%s'\n", argv[0], err.data.parser->expect, err.data.parser->got);
            // }
        }
        if (commands) free(commands);
        arena_free(&main_arena);
    }
cleanup:
    arena_free(&main_arena);
quit_no_cleanup:
    return ret;
}

#define ARENA_IMPLEMENTATION
#include "../arena.h"
