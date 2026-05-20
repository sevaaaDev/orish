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
            printf("%s: %s: command not found", prog_name, argv[0]);
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


struct Source {
    char *start;
    char *cur;
    size_t len;
};

// source is char*, so we can read from file and from -c argument
int
get_line(struct Source *source, char **out_line)
{
    char *line = NULL;
    if (source == NULL) {
        int ggets_err;
        do {
            printf("orish> ");
            ggets_err = ggets(&line);
            if (ggets_err == EOF) {
                printf("\nEOF\n");
                return EOF;
            };
            assert(line);
        } while(*line == '\0');
        *out_line = line;
        return ggets_err; 
    }
    if (source) {
        if (source->cur - source->start >= source->len) return EOF;
        char *start;
        do {
            char *cur = source->cur;
            start = cur;
            while (*cur != '\0' && *cur != '\n') { 
                cur++;
            }
            *cur = '\0';
            source->cur = cur+1;
        } while (*start == '\0');
        *out_line = start;
    }
    return 0;
}

int
main(int argc, char **argv)
{
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
    struct Source *src_handler = NULL;
    struct Source real_src = {0};
    if (flags.cmd_string) {
        real_src.start = flags.cmd_string;
        real_src.cur = flags.cmd_string;
        real_src.len = strlen(flags.cmd_string);
        src_handler = &real_src;
    }
    if (flags.filename) {
        int fd = open(flags.filename, O_RDWR);
        if (fd <= -1) {
            printf("%s: internal error: %s\n", argv[0], strerror(errno));
            ret = 1;
            goto quit_no_cleanup;
        }
        struct stat stat;
        fstat(fd, &stat);
        char *commands = mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
        close(fd);
        if (commands == MAP_FAILED) {
            printf("%s: internal error: %s\n", argv[0], strerror(errno));
            ret = 1;
            goto quit_no_cleanup;
        }
        real_src.start = commands;
        real_src.cur = commands;
        real_src.len = stat.st_size;
        src_handler = &real_src;
        prog_name = flags.filename;
    }
    bool running = true;
    while (running) {
        char *commands = NULL;
        if (setjmp(main_jump) == 0) {
            int lineerr = get_line(src_handler, &commands);
            if (lineerr == EOF) {
                running = false;
                continue;
            };
            if (lineerr) exit(34);
            struct Lexer lexer = lexer_new(commands);
            struct Parser parser = parser_new(&lexer, &main_jump, &second_arena, &main_arena);
            struct Node *root = parse(&parser);
            int status = exec_cmd(root, &ctx);
        } else {
            if (!flags.interactive) running = false;
        };
        if (flags.interactive && commands) free(commands);
        arena_reset(&main_arena);
        arena_reset(&second_arena);
    }
cleanup:
    if (flags.filename) {
        if (munmap(real_src.start, real_src.len) <= -1) {
            printf("%s: internal error %s\n", argv[0], strerror(errno));
        }
    }
    arena_free(&main_arena);
    arena_free(&second_arena);
quit_no_cleanup:
    return ret;
}

#define ARENA_IMPLEMENTATION
#include "../arena.h"
