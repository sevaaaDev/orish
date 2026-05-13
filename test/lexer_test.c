#include <criterion/criterion.h>
#include <string.h>

#include "../arena.h"
#include "../src/lexer.h"

#define ASSERT_EQ(a, b) cr_assert(a == b)
#define ASSERT_TRUE(a) cr_assert(a == true)
#define ASSERT_STR_EQ(a, b) cr_assert(strcmp(a, b) == 0)

Arena a = {0};
void teardown() {
    arena_free(&a);
}

TestSuite(lexer, .fini = teardown);

Test(lexer, accept_valid_input) {
    struct Lexer lex = lexer_new("echo hello;");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_EQ(tok->type, 2);
    tok = lexer_scan(&a, &lex);
    ASSERT_EQ(tok->type, 2);
    tok = lexer_scan(&a, &lex);
    ASSERT_EQ(tok->type, 0);
}
Test(lexer, accept_quoted_input) {
    struct Lexer lex = lexer_new("echo 'hello world';");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_EQ(tok->type, 2);
    tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ("'hello world'", tok->value);
    tok = lexer_scan(&a, &lex);
    ASSERT_EQ(tok->type, 0);
}
Test(lexer, accept_quoted_input_with_semicolon_inside) {
    struct Lexer lex = lexer_new("'hello; world'");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ(tok->value, "'hello; world'");
    tok = lexer_scan(&a, &lex);
    ASSERT_TRUE(!tok);
}
Test(lexer, quoted_token_shall_not_be_delimited_after_quote_ends) {
    struct Lexer lex = lexer_new("'hello 'world");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ(tok->value, "'hello 'world");
    tok = lexer_scan(&a, &lex);
    ASSERT_TRUE(!tok);
}
Test(lexer, quoted_and_unquoted_token_may_stick_as_one_token) {
    struct Lexer lex = lexer_new("hello' world'");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ(tok->value, "hello' world'");
    tok = lexer_scan(&a, &lex);
    ASSERT_TRUE(!tok);
}
Test(lexer, accept_backslash_as_escape) {
    struct Lexer lex = lexer_new("hello\\ world");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ(tok->value, "hello\\ world");
    tok = lexer_scan(&a, &lex);
    ASSERT_TRUE(!tok);
}
Test(lexer, escape_semicolon_using_backslash) {
    struct Lexer lex = lexer_new("hello\\;world");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ(tok->value, "hello\\;world");
    tok = lexer_scan(&a, &lex);
    ASSERT_TRUE(!tok);
}
Test(lexer, escape_dquotes_using_backslash) {
    struct Lexer lex = lexer_new("\"hello\\\" world\"");
    struct Token *tok = lexer_scan(&a, &lex);
    ASSERT_STR_EQ(tok->value, "\"hello\\\" world\"");
    tok = lexer_scan(&a, &lex);
    ASSERT_TRUE(!tok);
}

Test(lexer, remove_backslash_quoting) {
    char *token = "hello\\\\world"; 
    char *clean = lexer_util_quote_remover(&a, token);
    ASSERT_STR_EQ(clean, "hello\\world");
}
Test(lexer, remove_single_quoting) {
    char *token = "'hello \\'world"; 
    char *clean = lexer_util_quote_remover(&a, token);
    ASSERT_STR_EQ(clean, "hello \\world");
}
Test(lexer, remove_double_quoting) {
    char *token = "\"hello \"world"; 
    char *clean = lexer_util_quote_remover(&a, token);
    ASSERT_STR_EQ(clean, "hello world");
}
Test(lexer, remove_single_quoting_in_complex_string) {
    char *token = "'\"hello\" \\'world"; 
    char *clean = lexer_util_quote_remover(&a, token);
    ASSERT_STR_EQ(clean, "\"hello\" \\world");
}
Test(lexer, remove_double_quoting_in_complex_string) {
    char *token = "\"'hello' \\\"\\h\"world"; 
    char *clean = lexer_util_quote_remover(&a, token);
    ASSERT_STR_EQ(clean, "'hello' \"\\hworld");
}

#define ARENA_IMPLEMENTATION
#include "../arena.h"
