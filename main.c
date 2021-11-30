#include "./9cc.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

char *read_file(char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        error_at("cannot open %s: %s", path);

    if (fseek(fp, 0, SEEK_END) == -1)
        error_at("%s: fseek: %s", path);
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1)
        error_at("%s: fseek: %s", path);

    char *buf = calloc(1, size + 2);
    fread(buf, size, 1, fp);

    if (size == 0 || buf[size - 1] != '\n')
        buf[size++] = '\n';
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    file_name = argv[1];
    user_input = read_file(file_name);
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