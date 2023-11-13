#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_kind {
    TK_IDENT, // identifiers
    TK_PUNCT, // keywords or punctuators
    TK_NUM,   // numeric literals
    TK_EOF    // end-of-file markers
};

struct token {
    enum token_kind kind; // token kind
    struct token *next;   // next token
    int val;              // if kind is TK_NUM, its value
    char *loc;            // token location
    int len;              // token length
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(struct token *tok, char *fmt, ...);
bool equal(struct token *tok, char *str);
struct token *skip(struct token *tok, char *str);
struct token *tokenize(char *input);

struct obj {
    struct obj *next;
    char *name;
    int offset;
};

struct function {
    struct node *body;
    struct obj *locals;
    int stack_size;
};

enum node_kind {
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_NEG,       // unary -
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_ASSIGN,    // =
    ND_EXPR_STMT, // expression statement
    ND_VAR,       // variable
    ND_NUM        // integer
};

struct node {
    enum node_kind kind; // node kind
    struct node *next;   // next node
    struct node *lhs;    // left-hand side
    struct node *rhs;    // right-hand side
    struct obj *var;     // used if kind == ND_VAR
    int val;             // used if kind == ND_NUM
};

struct function *parse(struct token *tok);

void codegen(struct function *prog);
