#include "./9cc.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize();
    program();

    printf(".intel_syntax noprefix\n");

    if (globals->name != NULL)
    {
        printf(".data\n");
        for (LVar *var = globals; var->name != NULL; var = var->next)
        {
            char *name = malloc(globals->len + 1);
            strncpy(name, globals->name, globals->len);
            printf(".comm %s,%d,%d\n", name, globals->size, globals->size);
            free(name);
        }
    }

    printf(".text\n");

    if (strings->name != NULL)
    {
        for (LVar *var = strings; var->name != NULL; var = var->next)
        {
            printf(".LC%d:\n", var->offset);
            printf("    .string \"%s\"\n", var->name);
        }
    }

    printf(".globl main\n");
    for (int i = 0; code[i]; i++)
    {
        gen(code[i]);
    }
}