#include "rvcc.h"

struct obj *locals;

static struct node *expr_stmt(struct token **rest, struct token *tok);
static struct node *expr(struct token **rest, struct token *tok);
static struct node *assign(struct token **rest, struct token *tok);
static struct node *equality(struct token **rest, struct token *tok);
static struct node *relational(struct token **rest, struct token *tok);
static struct node *add(struct token **rest, struct token *tok);
static struct node *mul(struct token **rest, struct token *tok);
static struct node *unary(struct token **rest, struct token *tok);
static struct node *primary(struct token **rest, struct token *tok);

static struct obj *find_var(struct token *tok)
{
    for (struct obj *var = locals; var; var = var->next)
        if (strlen(var->name) == tok->len && 
            !strncmp(tok->loc, var->name, tok->len))
            return var;

    return NULL;
}

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

static struct node *new_var_node(struct obj *var)
{
    struct node *nd = new_node(ND_VAR);
    nd->var = var;

    return nd;
}

static struct obj *new_lvar(char *name)
{
    struct obj *var = calloc(1, sizeof(struct obj));
    var->name = name;
    var->next = locals;
    locals = var;

    return var;
}

// stmt = expr_stmt
static struct node *stmt(struct token **rest, struct token *tok)
{
    return expr_stmt(rest, tok);
}

// expr_stmt = expr ";"
static struct node *expr_stmt(struct token **rest, struct token *tok)
{
    struct node *nd = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");

    return nd;
}

// expr = assign
static struct node *expr(struct token **rest, struct token *tok)
{
    return assign(rest, tok);
}

// assign = equality ("=" assign)?
static struct node *assign(struct token **rest, struct token *tok)
{
    // equality
    struct node *nd = equality(&tok, tok);

    // ("=" assign)?
    if (equal(tok, "="))
        nd = new_binary(ND_ASSIGN, nd, assign(&tok, tok->next));

    *rest = tok;

    return nd;
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

// primary = "(" expr ")" | ident | num
static struct node *primary(struct token **rest, struct token *tok)
{
    // "(" expr ")"
    if (equal(tok, "(")) {
        struct node *nd = expr(&tok, tok->next);

        *rest = skip(tok, ")");

        return nd;
    }

    // ident
    if (tok->kind == TK_IDENT) {
        struct obj *var = find_var(tok);
        if (!var)
            var = new_lvar(strndup(tok->loc, tok->len));

        *rest = tok->next;

        return new_var_node(var);
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

// program = stmt*
struct function *parse(struct token *tok)
{
    struct node head = {};
    struct node *cur = &head;

    // stmt*
    while (tok->kind != TK_EOF) {
        cur->next = stmt(&tok, tok);
        cur = cur->next;
    }

    struct function *prog = calloc(1, sizeof(struct function));
    prog->body = head.next;
    prog->locals = locals;

    return prog;
}
