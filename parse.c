#include "rvcc.h"

static struct node *expr(struct token **rest, struct token *tok);
static struct node *equality(struct token **rest, struct token *tok);
static struct node *relational(struct token **rest, struct token *tok);
static struct node *add(struct token **rest, struct token *tok);
static struct node *mul(struct token **rest, struct token *tok);
static struct node *unary(struct token **rest, struct token *tok);
static struct node *primary(struct token **rest, struct token *tok);

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

// expr = equality
static struct node *expr(struct token **rest, struct token *tok)
{
    return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static struct node *equality(struct token **rest, struct token *tok)
{
    // relational
    struct node *nd = relational(&tok, tok);

    // ("==" relational | "!=" relational)*
    while (true) /* for (;;) */ {
        // "==" relational
        if (equal(tok, "==")) {
            nd = new_binary(ND_EQ, nd, relational(&tok, tok->next));
            continue;
        }

        // "!=" relational
        if (equal(tok, "!=")) {
            nd = new_binary(ND_NE, nd, relational(&tok, tok->next));
            continue;
        }

        *rest = tok;

        return nd;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static struct node *relational(struct token **rest, struct token *tok)
{
    // add
    struct node *nd = add(&tok, tok);

    // ("<" add | "<=" add | ">" add | ">=" add)*
    while (true) /* for (;;) */ {
        // "<" add
        if (equal(tok, "<")) {
            nd = new_binary(ND_LT, nd, add(&tok, tok->next));
            continue;
        }

        // "<=" add
        if (equal(tok, "<=")) {
            nd = new_binary(ND_LE, nd, add(&tok, tok->next));
            continue;
        }

        // ">" add
        if (equal(tok, ">")) {
            nd = new_binary(ND_LT, add(&tok, tok->next), nd);
            continue;
        }

        // ">=" add
        if (equal(tok, ">=")) {
            nd = new_binary(ND_LE, add(&tok, tok->next), nd);
            continue;
        }

        *rest = tok;

        return nd;
    }
}

// add = mul ("+" mul | "-" mul)*
static struct node *add(struct token **rest, struct token *tok)
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

struct node *parse(struct token *tok)
{
    struct node *nd = expr(&tok, tok);

    if (tok->kind != TK_EOF)
        error_tok(tok, "extra token");
    
    return nd;
}
