#include "rvcc.h"

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

static void gen_addr(struct node *nd)
{
    if (nd->kind == ND_VAR) {
        int offset = (nd->name - 'a' + 1) * 8;

        printf("    addi a0, fp, %d\n", -offset);

        return;
    }

    error("not a lvalue");
}

static void gen_expr(struct node *nd)
{
    switch (nd->kind) {
    case ND_NUM:
        // li rd, immediate: RV32I: lui and/or addi; RV64I: lui, addi, slli, addi, slli, addi, slli, addi
        printf("    li a0, %d\n", nd->val);
        return;
    case ND_NEG:
        // neg rd, rs: sub rd, x0, rs
        gen_expr(nd->lhs);
        printf("    neg a0, a0\n");
        return;
    case ND_VAR:
        gen_addr(nd);
        printf("    ld a0, 0(a0)\n");
        return;
    case ND_ASSIGN:
        gen_addr(nd->lhs);
        push();
        gen_expr(nd->rhs);
        pop("a1");
        printf("    sd a0, 0(a1)\n");
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
    case ND_EQ:
    case ND_NE:
        printf("    xor a0, a0, a1\n");
        if (nd->kind == ND_EQ)
            // seqz rd, rs: sltiu rd, rs, 1
            printf("    seqz a0, a0\n");
        else
            // snez rd, rs: sltu rd, x0, rs
            printf("    snez a0, a0\n");
        return;
    case ND_LT:
        printf("    slt a0, a0, a1\n");
        return;
    case ND_LE:
        printf("    slt a0, a1, a0\n");
        printf("    xori a0, a0, 1\n");
        return;
    default:
        break;
    }

    error("invalid expression");
}

static void gen_stmt(struct node *nd)
{
    if (nd->kind == ND_EXPR_STMT) {
        gen_expr(nd->lhs);
        return;
    }

    error("invalid statement");
}

void codegen(struct node *nd)
{
    printf("    .globl main\n");
    printf("main:\n");

    // stack
    //-------------------------------// sp
    //              fp                  fp = sp - 8
    //-------------------------------// fp
    //              'a'                 fp - 8
    //              'b'                 fp - 16
    //              ...
    //              'z'                 fp - 208
    //-------------------------------// sp = sp - 8 - 208
    //              ...
    //-------------------------------//

    // prologue
    printf("    addi sp, sp, -8\n");
    printf("    sd fp, 0(sp)\n");
    // mv rd, rs: addi rd, rs, 0
    printf("    mv fp, sp\n");

    printf("    addi sp, sp, -208\n");

    for (struct node *n = nd; n; n = n->next) {
        gen_stmt(n);
        assert(depth == 0);
    }

    // epilogue
    // mv rd, rs: addi rd, rs, 0
    printf("    mv sp, fp\n");
    printf("    ld fp, 0(sp)\n");
    printf("    addi sp, sp, 8\n");

    printf("    ret\n");
}
