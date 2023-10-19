#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_kind {
    TK_PUNCT,
    TK_NUM,
    TK_EOF
};

struct token {
    enum token_kind kind;
    struct token *next;
    int val;
    char *loc;
    int len;
};

static char * current_input;

static void error(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(1);
}

static void verror_at(char *loc, char *fmt, va_list ap)
{
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

static void error_at(char *loc, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
    va_end(ap);

    exit(1);
}

static void error_tok(struct token *tok, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
    va_end(ap);

    exit(1);
}

static bool equal(struct token *tok, char *str)
{
    return memcmp(tok->loc, str, tok->len) == 0 && str[tok->len] == '\0';
}

static struct token *skip(struct token *tok, char *str)
{
    if (!equal(tok, str))
        error_tok(tok, "expect %s", str);
        
    return tok->next;
}

static int get_number(struct token *tok)
{
    if (tok->kind != TK_NUM)
        error_tok(tok, "expect a number");

    return tok->val;
}

static struct token *new_token(enum token_kind kind, char *start, char *end)
{
    struct token *tok = calloc(1, sizeof(struct token));

    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    
    return tok;
}

static struct token *tokenize(void)
{
    char *p = current_input;
    struct token head = {};
    struct token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (isdigit(*p)) {
            cur->next = new_token(TK_NUM, p, p);
            cur = cur->next;
            const char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (ispunct(*p)) {
            cur->next = new_token(TK_PUNCT, p, p + 1);
            cur = cur->next;
            ++p;
            continue;
        }

        error_at(p, "invalid token");
    }

    cur->next = new_token(TK_EOF, p, p);

    return head.next;
}

enum node_kind {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NEG,
    ND_NUM
};

struct node {
    enum node_kind kind;
    struct node *lhs;    // left-hand side
    struct node *rhs;    // right-hand side
    int val;
};

static struct node *new_node(enum node_kind kind)
{
    struct node *nd = calloc(1, sizeof(struct node));

    nd->kind = kind;

    return nd;
}

static struct node *new_unary(enum node_kind kind, struct node *expr)
{
    struct node *nd = new_node(kind);

    nd->lhs = expr;

    return nd;
}

static struct node *new_binary(enum node_kind kind, struct node *lhs, struct node *rhs)
{
    struct node *nd = new_node(kind);

    nd->lhs = lhs;
    nd->rhs = rhs;
    
    return nd;
}

static struct node *new_num(int val)
{
    struct node *nd = new_node(ND_NUM);

    nd->val = val;

    return nd;
}

static struct node *expr(struct token **rest, struct token *tok);
static struct node *mul(struct token **rest, struct token *tok);
static struct node *unary(struct token **rest, struct token *tok);
static struct node *primary(struct token **rest, struct token *tok);

// expr = mul ("+" mul | "-" mul)*
static struct node *expr(struct token **rest, struct token *tok)
{
    // mul
    struct node *nd = mul(&tok, tok);

    // ("+" mul | "-" mul)*
    while (true) /* for (;;) */ {
        // "+" mul
        if (equal(tok, "+")) {
            nd = new_binary(ND_ADD, nd, mul(&tok, tok->next));
            continue;
        }

        // "-" mul
        if (equal(tok, "-")) {
            nd = new_binary(ND_SUB, nd, mul(&tok, tok->next));
            continue;
        }

        *rest = tok;

        return nd;
    }
}

// mul = unary ("*" unary | "/" unary)*
static struct node *mul(struct token **rest, struct token *tok)
{
    // unary
    struct node *nd = unary(&tok, tok);

    // ("*" unary | "/" unary)*
    while (true) /* for (;;) */ {
        // "*" unary
        if (equal(tok, "*")) {
            nd = new_binary(ND_MUL, nd, unary(&tok, tok->next));
            continue;
        }

        // "/" unary
        if (equal(tok, "/")) {
            nd = new_binary(ND_DIV, nd, unary(&tok, tok->next));
            continue;
        }

        *rest = tok;

        return nd;
    }
}

// unary = ("+" | "-") unary | primary
static struct node *unary(struct token **rest, struct token *tok)
{
    // "+" unary
    if (equal(tok, "+"))
        return unary(rest, tok->next);

    // "-" unary
    if (equal(tok, "-"))
        return new_unary(ND_NEG, unary(rest, tok->next));

    // primary
    return primary(rest, tok);
}

// primary = "(" expr ")" | num
static struct node *primary(struct token **rest, struct token *tok)
{
    // "(" expr ")"
    if (equal(tok, "(")) {
        struct node *nd = expr(&tok, tok->next);

        *rest = skip(tok, ")");
        
        return nd;
    }

    // num
    if (tok->kind == TK_NUM) {
        struct node *nd = new_num(tok->val);

        *rest = tok->next;

        return nd;
    }

    error_tok(tok, "expected an expression");

    return NULL;
}

static int depth;

static void push(void)
{
    printf("    addi sp, sp, -8\n");
    printf("    sd a0, 0(sp)\n");
    depth++;
}

static void pop(char *reg)
{
    printf("    ld %s, 0(sp)\n", reg);
    printf("    addi sp, sp, 8\n");
    depth--;
}

static void gen_expr(struct node *nd)
{
    switch (nd->kind) {
    case ND_NUM:
        printf("    li a0, %d\n", nd->val);
        return;
    case ND_NEG:
        gen_expr(nd->lhs);
        printf("    neg a0, a0\n");
        return;
    default:
        break;
    }

    gen_expr(nd->rhs);
    push();
    gen_expr(nd->lhs);
    pop("a1");

    switch (nd->kind) {
    case ND_ADD:
        printf("    add a0, a0, a1\n");
        return;
    case ND_SUB:
        printf("    sub a0, a0, a1\n");
        return;
    case ND_MUL:
        printf("    mul a0, a0, a1\n");
        return;
    case ND_DIV:
        printf("    div a0, a0, a1\n");
        return;
    default:
        break;
    }

    error("invalid expression");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        error("%s: invalid number of arguments\n", argv[0]);

    current_input = argv[1];
    struct token *tok = tokenize();

    struct node *node = expr(&tok, tok);

    if (tok->kind != TK_EOF)
        error_tok(tok, "extra token");

    printf("    .globl main\n");
    printf("main:\n");

    gen_expr(node);

    printf("    ret\n");

    assert(depth == 0);

    return 0;
}
