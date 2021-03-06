#include "9cc.h"

Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

bool startswith(char *p, char *q)
{
    return memcmp(p, q, strlen(q)) == 0;
}

Token *tokenize()
{
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p)
    {
        if (isspace(*p) || *p == '\n')
        {
            p++;
            continue;
        }

        if (strncmp(p, "//", 2) == 0)
        {
            p += 2;
            while (*p != '\n')
                p++;
            continue;
        }

        if (strncmp(p, "/*", 2) == 0)
        {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "コメントが閉じられていません");
            p = q + 2;
            continue;
        }

        if (!strncmp(p, "sizeof", 6) && !is_alnum(p[6]))
        {
            cur = new_token(TK_SIZEOF, cur, p, 6);
            p += 6;
        }

        if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3]))
        {
            p += 4;
            Type *type = (Type *)calloc(1, sizeof(Type));
            type->ty = INT;
            while (*p == '*')
            {
                Type *tmp = (Type *)calloc(1, sizeof(Type));
                tmp->ty = PTR;
                tmp->ptr_to = type;
                type = tmp;
                p++;
            }
            cur = new_token(TK_NEW_IDENT, cur, p, 0);
            cur->ty = type;
            while (is_alnum(*p))
            {
                cur->len++;
                p++;
            }
            if (!strncmp(p, "[", 1) && isdigit(p[1]))
            {
                p++;
                Type *new = (Type *)calloc(1, sizeof(Type));
                new->ty = ARRAY;
                new->array_size = strtol(p, &p, 10);
                new->ptr_to = type;
                cur->ty = new;
                p++;
            }
        }

        if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4]))
        {
            p += 5;
            Type *type = (Type *)calloc(1, sizeof(Type));
            type->ty = CHAR;
            while (*p == '*')
            {
                Type *tmp = (Type *)calloc(1, sizeof(Type));
                tmp->ty = PTR;
                tmp->ptr_to = type;
                type = tmp;
                p++;
            }
            cur = new_token(TK_NEW_IDENT, cur, p, 0);
            cur->ty = type;
            while (is_alnum(*p))
            {
                cur->len++;
                p++;
            }
            if (!strncmp(p, "[", 1) && isdigit(p[1]))
            {
                p++;
                Type *new = (Type *)calloc(1, sizeof(Type));
                new->ty = ARRAY;
                new->array_size = strtol(p, &p, 10);
                new->ptr_to = type;
                cur->ty = new;
                p++;
            }
            continue;
        }

        if (strncmp(p, "T", 1) == 0 && !is_alnum(p[1]))
        {
            p += 2;
            Type *type = (Type *)calloc(1, sizeof(Type));
            type->ty = GENERIC;
            while (*p == '*')
            {
                Type *tmp = (Type *)calloc(1, sizeof(Type));
                tmp->ty = PTR;
                tmp->ptr_to = type;
                type = tmp;
                p++;
            }
            cur = new_token(TK_NEW_IDENT, cur, p, 0);
            cur->ty = type;
            while (is_alnum(*p))
            {
                cur->len++;
                p++;
            }
            if (!strncmp(p, "[", 1) && isdigit(p[1]))
            {
                p++;
                Type *new = (Type *)calloc(1, sizeof(Type));
                new->ty = ARRAY;
                new->array_size = strtol(p, &p, 10);
                new->ptr_to = type;
                cur->ty = new;
                p++;
            }
            continue;
        }

        if (startswith(p, "\""))
        {
            cur = new_token(TK_STRING, cur, p, 0);
            for (++p; *p != '"'; p++)
                cur->len++;
            p++;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6]))
        {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2]))
        {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4]))
        {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5]))
        {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3]))
        {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") ||
            startswith(p, ">=") || startswith(p, "<="))
        {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>;={},&[]", *p))
        {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (isalpha(*p))
        {
            cur = new_token(TK_IDENT, cur, p, 0);
            while (is_alnum(*p))
            {
                cur->len++;
                p++;
            }
            continue;
        }

        error_at(p, "トークンにできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

void error_at(char *loc, char *msg)
{
    char *line = loc;
    while (user_input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n')
        end++;

    int line_num = 1;
    for (char *p = user_input; p < line; p++)
        if (*p == '\n')
            line_num++;

    int indent = fprintf(stderr, "%s:%d: ", file_name, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

bool consume(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

bool judge(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;
    return true;
}

bool check(char op)
{
    if (*(token->str) == op)
        return true;
    else
        return false;
}

bool consume_kind(TokenKind kind)
{
    if (token->kind == kind)
    {
        token = token->next;
        return true;
    }
    return false;
}

Token *consume_ident()
{
    Token *tok = token;
    if (tok->kind != TK_IDENT && tok->kind != TK_NEW_IDENT)
        return NULL;
    token = token->next;
    return tok;
}

void expect(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        error_at(token->str, "予期しない記号です");
    token = token->next;
}

int expect_number()
{
    if (token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof()
{
    return token->kind == TK_EOF;
}

LVar *find_lvar(Token *tok)
{
    for (LVar *var = locals; var; var = var->next)
    {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}

LVar *find_global_lvar(Token *tok)
{
    for (LVar *var = globals; var; var = var->next)
    {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}

LVar *find_global_lvar_offset(int offset)
{
    for (LVar *var = globals; var; var = var->next)
    {
        if (var->offset == offset)
            return var;
    }
    return NULL;
}

int is_alnum(char c)
{
    return (isalpha(c) || isdigit(c) || c == '_');
}

bool is_global(Node *node)
{
    for (Node *tmp = node; tmp->lhs; tmp = tmp->lhs)
    {
        if (tmp->kind == ND_GLOBAL)
            return true;
    }
    return false;
}