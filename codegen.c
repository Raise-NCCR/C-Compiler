#include "./9cc.h"

static int jump_number = 0;
static int arg_count = 0;
static int global_lvar_offset = 1;
int deref_count = 0;
static bool block = false;
static bool is_in_block = false;
static char *arg_register[] = {"rdi", "rsi", "rdx", "rcx", "r8"};
static LVar *func;
Type *type = NULL;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

void program()
{
    int i = 0;
    locals = calloc(1, sizeof(LVar));
    globals = calloc(1, sizeof(LVar));
    while (!at_eof())
        code[i++] = stmt();
    code[i] = NULL;
}

Node *stmt()
{
    Node *node;

    if (consume_kind(TK_RETURN))
    {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    }
    else if (consume("{"))
    {
        node = stmt();
        for (;;)
        {
            if (consume("}"))
            {
                block = true;
                is_in_block = false;
                return node;
            }
            else
                node = new_node(ND_BLOCK, node, stmt());
        }
    }
    else if (consume_kind(TK_IF))
    {
        expect("(");
        node = expr();
        expect(")");
        node = new_node(ND_IF, node, stmt());
        for (;;)
        {
            if (consume_kind(TK_ELSE))
                node = new_node(ND_ELSE, node, stmt());
            else
                return node;
        }
    }
    else if (consume_kind(TK_WHILE))
    {
        expect("(");
        node = expr();
        expect(")");
        node = new_node(ND_WHILE, node, stmt());
        return node;
    }
    else if (consume_kind(TK_FOR))
    {
        expect("(");
        int flag = 0;
        for (;;)
        {

            if (consume(";"))
            {
                if (!flag++)
                    node = NULL;
                else
                    node = new_node(ND_FOR, node, NULL);
                continue;
            }
            else if (consume(")"))
            {
                if (!flag++)
                    node = NULL;
                node = new_node(ND_FOR, node, stmt());
                return node;
            }
            else
            {
                if (!flag++)
                    node = expr();
                else
                    node = new_node(ND_FOR, node, expr());
                consume(";");
            }
        }
    }
    else
    {
        node = expr();
    }

    if (!consume(";"))
    {
        if (!block)
            error_at(token->str, "';'ではないトークンです");
    }
    return node;
}

Node *expr()
{
    return assign();
}

Node *assign()
{
    Node *node = equality();

    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

Node *equality()
{
    Node *node = relational();

    for (;;)
    {
        if (consume("=="))
            node = new_node(ND_EQU, node, relational());
        else if (consume("!="))
            node = new_node(ND_NEQ, node, relational());
        else
            return node;
    }
}

Node *relational()
{
    Node *node = add();

    for (;;)
    {
        if (consume("<"))
            node = new_node(ND_LES, node, add());
        else if (consume("<="))
            node = new_node(ND_LEQ, node, add());
        else if (consume(">"))
            node = new_node(ND_LES, add(), node);
        else if (consume(">="))
            node = new_node(ND_LEQ, add(), node);
        else
            return node;
    }
}

Node *add()
{
    Node *node = mul();

    for (;;)
    {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul()
{
    Node *node = unary();

    for (;;)
    {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary()
{
    Node *node;
    if (consume("+"))
        return primary();
    else if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), unary());
    else if (consume("&"))
        return new_node(ND_ADDR, unary(), NULL);
    else if (consume("*"))
        return new_node(ND_DEREF, unary(), NULL);
    else if (consume_kind(TK_SIZEOF))
    {
        node = unary();
    }
    return primary();
}

Node *primary()
{
    Token *tok = consume_ident();
    if (tok)
    {
        Node *node = calloc(1, sizeof(Node));
        if (consume("("))
        {
            node->kind = ND_FUNC;
            for (;;)
            {
                is_in_block = true;
                if (consume(","))
                    continue;
                else if (consume(")"))
                    break;
                else
                    node = new_node(ND_FUNC, node, unary());
            }
        }
        else
        {
            node->kind = ND_LVAR;
        }
        LVar *lvar;
        if (tok->kind == TK_NEW_IDENT && is_in_block)
        {
            int offset = 0;
            node->kind = ND_LVAR;
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->ty = tok->ty;
            if (lvar->ty->ty == ARRAY)
            {
                if (lvar->ty->ptr_to->ty == INT)
                    offset = 4 * ((lvar->ty->array_size / 2 + 1) * 2);
                if (lvar->ty->ptr_to->ty == PTR)
                    offset = 8 * lvar->ty->array_size;
            }
            lvar->offset = locals->offset + 8 + offset;
            node->offset = lvar->offset;
            node->ty = tok->ty;
            if (lvar->ty->ty == ARRAY)
            {
                node->ty->ty = PTR;
                Node *tmp = calloc(1, sizeof(Node));
                tmp->offset = node->offset - 8;
                tmp->kind = ND_LVAR;
                tmp = new_node(ND_ADDR, tmp, NULL);
                node = new_node(ND_ASSIGN, node, tmp);
            }
            locals = lvar;
        }
        else if (tok->kind == TK_NEW_IDENT && !is_in_block && !judge("("))
        {
            lvar = calloc(1, sizeof(LVar));
            node->kind = ND_GLOBAL;
            lvar->next = globals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->ty = tok->ty;
            lvar->offset = global_lvar_offset;
            global_lvar_offset++;
            lvar->size = 4;
            node->ty = lvar->ty;
            if (lvar->ty->ty == ARRAY)
            {
                if (lvar->ty->ptr_to->ty == INT)
                    lvar->size = 4 * lvar->ty->array_size;
                if (lvar->ty->ptr_to->ty == PTR)
                    lvar->size = 8 * lvar->ty->array_size;
                node->ty->ty = PTR;
            }
            else if (lvar->ty->ty == PTR) lvar->size = 8;
            globals = lvar;
            if (consume("["))
            {
                expect_number();
                consume("]");
            }
            return node;
        }
        else
        {
            lvar = find_lvar(tok);
            if (lvar)
            {
                node->ty = lvar->ty;
                node->offset = lvar->offset;
            }
            else
            {
                lvar = find_global_lvar(tok);
                if (lvar)
                {
                    node->ty = lvar->ty;
                    node->offset = -1 * lvar->offset;
                }
                else error_at(tok->str, "定義されていない変数です");
            }
        }

        if (consume("["))
        {
            node = new_node(ND_ADD, node, new_node_num(expect_number()));
            node = new_node(ND_DEREF, node, NULL);
            expect("]");
        }

        if (judge("{"))
        {
            is_in_block = true;
            node = new_node(ND_DECLARE, node, stmt());
            node->offset = lvar->offset;
        }
        return node;
    }

    if (consume("("))
    {
        Node *node = expr();
        expect(")");
        return node;
    }

    return new_node_num(expect_number());
}

void gen_lvar(Node *node)
{
    if (node->kind != ND_LVAR)
        perror("代入の左辺値が変数ではありません");

    if (node->offset < 0)
    {
        int offset = -1 * node->offset;
        LVar *lvar = find_global_lvar_offset(offset);
        char *name = malloc(lvar->len + 1);
        strncpy(name, lvar->name, lvar->len);
        printf("    lea rax, %s[rip]\n", name);
        free(name);
    }
    else
    {
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", node->offset);
    }
    printf("    push rax\n");
    return ;
}

char *find_func(Node *node)
{
    for (LVar *var = locals; var; var = var->next)
    {
        if (var->offset == node->offset)
        {
            char *func_name = (char *)calloc(var->len + 1, 1);
            return strncpy(func_name, var->name, var->len);
        }
    }
    return NULL;
}

void gen_func(Node *node)
{
    if (node->rhs == NULL)
        return;
    if (node->lhs->lhs == NULL)
    {
        gen(node->rhs);
        printf("    pop %s\n", arg_register[arg_count++]);
        return;
    }
    gen_func(node->lhs);
    gen(node->rhs);
    printf("    pop %s\n", arg_register[arg_count++]);
    return;
}

void declare_func(Node *node)
{
    if (node->rhs == NULL || node->lhs == NULL)
        return;
    declare_func(node->lhs);
    gen_lvar(node->rhs);
    printf("    pop rax\n");
    printf("    mov [rax], %s\n", arg_register[arg_count++]);
    return;
}

bool valid(Node *node, int floor, ...)
{
    va_list ap;
    va_start(ap, floor);

    for (int i = 0; i < floor; i++)
    {
        char *input = va_arg(ap, char *);
        if (strncmp(input, "lhs", 3) == 0)
        {
            if (node->lhs)
                node = node->lhs;
            else
                return false;
        }
        else if (strncmp(input, "rhs", 3) == 0)
        {
            if (node->rhs)
                node = node->rhs;
            else
                return false;
        }
    }
    return true;
}

void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("    push %d\n", node->val);
        return;
    }
    if (node->kind == ND_LVAR)
    {
        gen_lvar(node);
        printf("    pop rax\n");
        if (node->ty->ty == CHAR) printf("movsx ecx, BYTE PTR [rax]\n");
        else printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    }
    if (node->kind == ND_DECLARE)
    {
        arg_count = 0;
        printf("%s:\n", find_func(node));
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, 208\n");
        declare_func(node->lhs);
        gen(node->rhs);
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    }
    if (node->kind == ND_FUNC)
    {
        arg_count = 0;
        gen_func(node);
        printf("    call %s\n", find_func(node));
        return;
    }
    if (node->kind == ND_ASSIGN)
    {
        if (node->lhs->kind == ND_DEREF)
            gen(node->lhs->lhs);
        else
            gen_lvar(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        if (node->lhs->ty && node->lhs->ty->ty == CHAR)
        {
            printf("    mov rcx, rdi\n");
            printf("    mov [rax], cl\n");
            printf("    push rcx\n");
        }
        else
        {
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
        }
        return;
    }
    if (node->kind == ND_RETURN)
    {
        gen(node->lhs);

        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    }
    if (node->kind == ND_IF)
    {
        int num = jump_number;
        jump_number++;
        gen(node->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", num);

        gen(node->rhs);

        printf(".Lend%d:\n", num++);
        return;
    }
    if (node->kind == ND_ELSE)
    {
        int num = jump_number;
        jump_number++;
        gen(node->lhs->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lelse%d\n", num);

        gen(node->lhs->rhs);

        printf("    jmp .Lend%d\n", num);
        printf(".Lelse%d:\n", num);

        gen(node->rhs);

        printf(".Lend%d:\n", num++);
        return;
    }
    if (node->kind == ND_WHILE)
    {
        int num = jump_number;
        jump_number++;
        printf(".Lbegin%d:\n", num);

        gen(node->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", num);

        gen(node->rhs);

        printf("    jmp .Lbegin%d\n", num);
        printf(".Lend%d:\n", num++);
        return;
    }
    if (node->kind == ND_FOR)
    {
        int num = jump_number;
        jump_number++;

        if (valid(node, 3, "lhs", "lhs", "lhs"))
            gen(node->lhs->lhs->lhs);

        printf(".Lbegin%d:\n", num);

        if (valid(node, 3, "lhs", "lhs", "rhs"))
        {
            gen(node->lhs->lhs->rhs);

            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .Lend%d\n", num);
        }

        gen(node->rhs);
        if (valid(node, 2, "lhs", "rhs"))
            gen(node->lhs->rhs);

        printf("    jmp .Lbegin%d\n", num);
        printf(".Lend%d:\n", num++);
        return;
    }
    if (node->kind == ND_BLOCK)
    {
        gen(node->lhs);

        printf("    pop rax\n");

        gen(node->rhs);
        return;
    }
    if (node->kind == ND_ADDR)
    {
        gen_lvar(node->lhs);
        return;
    }
    if (node->kind == ND_DEREF)
    {
        if (is_global(node))
            return;
        gen(node->lhs);
        deref_count++;
        printf("    pop rax\n");
        if (node->lhs->ty && node->lhs->ty->ty == CHAR) printf("movsx ecx, BYTE PTR [rax]\n");
        else printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    }
    if (node->kind == ND_GLOBAL)
    {
        return;
    }

    type = node->lhs->ty;

    if (node->lhs->offset < 0) gen_lvar(node->lhs);
    else gen(node->lhs);

    int size = 1;
    if (type != NULL)
    {
        if (type->ty == PTR)
        {
            if (deref_count)
            {
                while (deref_count-- > 0)
                    type = type->ptr_to;
            }
            if (type->ty == PTR && type->ptr_to->ty == INT)
                size = 4;
            else if (type->ty == PTR && type->ptr_to->ty == PTR)
                size = 8;
        }
    }
    deref_count = 0;
    type = NULL;

    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");
    printf("    imul rdi, %d\n", size);

    if (node->kind == ND_EQU)
    {
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_NEQ)
    {
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_LES)
    {
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_LEQ)
    {
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_ADD)
    {
        printf("    add rax, rdi\n");
    }
    if (node->kind == ND_SUB)
    {
        printf("    sub rax, rdi\n");
    }
    if (node->kind == ND_MUL)
    {
        printf("    imul rax, rdi\n");
    }
    if (node->kind == ND_DIV)
    {
        printf("    cqo\n");
        printf("    idiv rdi\n");
    }

    printf("    push rax\n");
}
