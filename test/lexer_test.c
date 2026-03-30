#include <string.h>
#include "../src/lexer.h"

// Reset
#define RESET "\033[0m"

// Regular text colors
#define BLACK   "\033[30m"
#define WHITE   "\033[37m"

// Bright text colors
#define BBLACK   "\033[90m"
#define BWHITE   "\033[97m"

// Bright background colors
#define BG_BBLACK   "\033[100m"
#define BG_BRED     "\033[101m"
#define BG_BGREEN   "\033[102m"
#define BG_BYELLOW  "\033[103m"
#define BG_BBLUE    "\033[104m"
#define BG_BMAGENTA "\033[105m"
#define BG_BCYAN    "\033[106m"
#define BG_BWHITE   "\033[107m"

// counters
static int tests_run = 0;
static int tests_failed = 0;

// test definition
#define TEST(name) void name(void)

// run test
#define RUN_TEST(test) \
    do { \
        tests_run++; \
        printf("Running %s...\n", #test); \
        test(); \
    } while (0)

// assert true
#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf(BG_BRED BWHITE " FAIL " RESET " %s\n  at %s:%d\n", \
                   #cond, __FILE__, __LINE__); \
        } else { \
            printf(BG_BCYAN BLACK "  OK  " RESET " "#cond" == true\n"); \
        } \
    } while (0)

// assert equal (int)
#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf(BG_BRED BWHITE " FAIL " RESET \
                   " %s == %s (got %d vs %d)\n  at %s:%d\n", \
                   #a, #b, (a), (b), __FILE__, __LINE__); \
        } else { \
            printf(BG_BCYAN BLACK "  OK  " RESET " "#a" == "#b"\n"); \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b))) { \
            printf(BG_BRED BWHITE " FAIL " RESET \
                   " %s == %s (got %s vs %s)\n  at %s:%d\n", \
                   #a, #b, (a), (b), __FILE__, __LINE__); \
        } else { \
            printf(BG_BCYAN BLACK "  OK  " RESET " %s == %s\n", #a, (b)); \
        } \
    } while (0)

// mark success (optional explicit)
#define TEST_PASS(msg) \
    do { \
        printf(BG_BGREEN BWHITE "  OK  " RESET "\n"); \
    } while (0)

#define test(expr, msg) do {\
    if (!(expr)) { \
        printf("    "BG_BRED BLACK" FAIL "RESET" "msg"\n"); \
        fprintf(stderr, \
            "Assertion failed: %s\nFile: %s\nLine: %d\nFunction: %s\n", \
            #expr, __FILE__, __LINE__, __func__); \
    } else { \
        printf("    "BG_BCYAN BLACK" OK "RESET" "msg"\n"); \
    } \
} while(0)


#define group(title) printf(title"\n"); if (1)

#define str_eq(a, b) !strcmp(a, b)

int
main()
{
    Arena a = {0};
    group("accept valid input") {
        struct Lexer lex = lexer_new("echo hello;");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_EQ(tok->type, 2);
        tok = lexer_scan(&a, &lex);
        ASSERT_EQ(tok->type, 2);
        tok = lexer_scan(&a, &lex);
        ASSERT_EQ(tok->type, 0);
    }
    group("accept quoted input") {
        struct Lexer lex = lexer_new("echo 'hello world';");
        struct Token *tok = lexer_scan(&a, &lex);
        test(tok->type == 2, "echo");
        tok = lexer_scan(&a, &lex);
        test(str_eq("'hello world'", tok->value), "'hello world'");
        tok = lexer_scan(&a, &lex);
        test(tok->type == 0, ";");
    }
    group("accept quoted input with semicolon inside") {
        struct Lexer lex = lexer_new("'hello; world'");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_STR_EQ(tok->value, "'hello; world'");
        tok = lexer_scan(&a, &lex);
        ASSERT_TRUE(!tok);
    }
    group("quoted token shall not be delimited after quote ends") {
        struct Lexer lex = lexer_new("'hello 'world");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_STR_EQ(tok->value, "'hello 'world");
        tok = lexer_scan(&a, &lex);
        ASSERT_TRUE(!tok);
    }
    group("quoted and unquoted token may stick as one token") {
        struct Lexer lex = lexer_new("hello' world'");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_STR_EQ(tok->value, "hello' world'");
        tok = lexer_scan(&a, &lex);
        ASSERT_TRUE(!tok);
    }
    group("accept backslash as escape") {
        struct Lexer lex = lexer_new("hello\\ world");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_STR_EQ(tok->value, "hello\\ world");
        tok = lexer_scan(&a, &lex);
        ASSERT_TRUE(!tok);
    }
    group("escape semicolon using backslash") {
        struct Lexer lex = lexer_new("hello\\;world");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_STR_EQ(tok->value, "hello\\;world");
        tok = lexer_scan(&a, &lex);
        ASSERT_TRUE(!tok);
    }
    group("escape dquotes using backslash") {
        struct Lexer lex = lexer_new("\"hello\\\" world\"");
        struct Token *tok = lexer_scan(&a, &lex);
        ASSERT_STR_EQ(tok->value, "\"hello\\\" world\"");
        tok = lexer_scan(&a, &lex);
        ASSERT_TRUE(!tok);
    }

    group("remove backslash quoting") {
        char *token = "hello\\\\world"; 
        char *clean = lexer_util_quote_remover(&a, token);
        ASSERT_STR_EQ(clean, "hello\\world");
    }
    group("remove single quoting") {
        char *token = "'hello \\'world"; 
        char *clean = lexer_util_quote_remover(&a, token);
        ASSERT_STR_EQ(clean, "hello \\world");
    }
    group("remove double quoting") {
        char *token = "\"hello \"world"; 
        char *clean = lexer_util_quote_remover(&a, token);
        ASSERT_STR_EQ(clean, "hello world");
    }
    group("remove single quoting in complex string") {
        char *token = "'\"hello\" \\'world"; 
        char *clean = lexer_util_quote_remover(&a, token);
        ASSERT_STR_EQ(clean, "\"hello\" \\world");
    }
    group("remove double quoting in complex string") {
        char *token = "\"'hello' \\\"\\h\"world"; 
        char *clean = lexer_util_quote_remover(&a, token);
        ASSERT_STR_EQ(clean, "'hello' \"\\hworld");
    }
    arena_free(&a);
}

#define ARENA_IMPLEMENTATION
#include "../arena.h"
