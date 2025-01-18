#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRLEN 128
#define MAX_FILE_STRLEN 128
#define MAX_ENUMS 128
#define MAX_ENUM_VALUES 128

char cfile[MAX_FILE_STRLEN];
char hfile[MAX_FILE_STRLEN];
char hfile_ifndef[MAX_FILE_STRLEN];

typedef struct {
    char* name;
    char* label;
    int val;
} Value;

typedef struct {
    char* name;
    char* prefix;
    Value* values;
    size_t valueslen;
} Enum;

Enum* enums;
size_t enumslen;

unsigned char* pool;
unsigned char* ptr;
void* allocate(size_t size)
{
    void* resptr = ptr;
    ptr += size;
    return resptr;
}

int read_enum_file(char* path)
{
    FILE* f = fopen(path, "r");
    if (f == NULL)
        return 1;

    int cur_value_index = 0;

    char line[MAX_STRLEN];
    while (fgets(line, sizeof(line), f) != NULL) {
        int linelen = strlen(line);

        int spaceidx = 0;
        for (; line[spaceidx] != ' ' && spaceidx < linelen; spaceidx++);

        if (strncmp("CFILE", line, strlen("CFILE")) == 0)
            memcpy(cfile, (char*)(line + spaceidx + 1), linelen - spaceidx - 2);

        if (strncmp("HFILE", line, strlen("HFILE")) == 0) {
            memcpy(hfile, (char*)(line + spaceidx + 1), linelen - spaceidx - 2);

            memcpy(hfile_ifndef, (char*)(line + spaceidx + 1), linelen - spaceidx - 2);
            for (int j = 0; hfile_ifndef[j] != '\0'; j++) {
                if (hfile_ifndef[j] == '.')
                    hfile_ifndef[j] = '_';
                else
                    hfile_ifndef[j] = toupper(hfile_ifndef[j]);
            }
        }

        if (strncmp("ENUM", line, strlen("ENUM")) == 0) {
            int spaceidx2 = spaceidx + 1;
            for (; line[spaceidx2] != ' ' && spaceidx2 < linelen; spaceidx2++);

            Enum e = {0};
            e.values = allocate(sizeof(Value) * MAX_ENUM_VALUES);

            e.name = allocate(sizeof(char) * MAX_STRLEN);
            memcpy(e.name, (char*)(line + spaceidx + 1), spaceidx2 - spaceidx - 1);

            e.prefix = allocate(sizeof(char) * MAX_STRLEN);
            memcpy(e.prefix, (char*)(line + spaceidx2 + 1), linelen - spaceidx2 - 2);

            enums[enumslen++] = e;
        }

        if (strncmp("VALUE", line, strlen("VALUE")) == 0) {
            int spaceidx2 = spaceidx + 1;
            for (; line[spaceidx2] != ' ' && spaceidx2 < linelen; spaceidx2++);

            Value v = {0};
            v.name = allocate(sizeof(char) * MAX_STRLEN);
            v.label = allocate(sizeof(char) * MAX_STRLEN);

            if (spaceidx2 == linelen) {
                memcpy(v.name, (char*)(line + spaceidx + 1), linelen - spaceidx - 2);
            } else {
                memcpy(v.name, (char*)(line + spaceidx + 1), spaceidx2 - spaceidx - 1);
                memcpy(v.label, (char*)(line + spaceidx2 + 1), linelen - spaceidx2 - 2);
            }

            enums[enumslen - 1].values[enums[enumslen - 1].valueslen++] = v;
        }

        if (strncmp("IVALUE", line, strlen("IVALUE")) == 0) {
            int spaceidx2 = spaceidx + 1;
            for (; line[spaceidx2] != ' ' && spaceidx2 < linelen; spaceidx2++);

            int spaceidx3 = spaceidx2 + 1;
            for (; line[spaceidx3] != ' ' && spaceidx3 < linelen; spaceidx3++);

            Value v = {0};

            char valstr[MAX_STRLEN];
            memcpy(valstr, (char*)(line + spaceidx + 1), linelen - spaceidx - 2);
            v.val = atoi(valstr);

            v.name = allocate(sizeof(char) * MAX_STRLEN);
            v.label = allocate(sizeof(char) * MAX_STRLEN);

            if (spaceidx3 == linelen) {
                memcpy(v.name, (char*)(line + spaceidx2 + 1), linelen - spaceidx2 - 2);
            } else {
                memcpy(v.name, (char*)(line + spaceidx2 + 1), spaceidx3 - spaceidx2 - 1);
                memcpy(v.label, (char*)(line + spaceidx3 + 1), linelen - spaceidx3 - 2);
            }

            enums[enumslen - 1].values[enums[enumslen - 1].valueslen++] = v;
        }
    }

    fclose(f);
    return 0;
}

int write_implementation(char* dir)
{
    char path[MAX_STRLEN];
    sprintf(path, "%s/%s", dir, cfile);

    FILE* f = fopen(path, "w");
    if (f == NULL)
        return 1;

    fprintf(f, "// GENERATED BY CENUM\n\n");
    fprintf(f, "#include \"%s\"\n\n", hfile);

    char decl_full_name[MAX_STRLEN];
    for (int i = 0; i < enumslen; i++) {
        fprintf(f, "char* toString_%s(%s value)\n", enums[i].name, enums[i].name);
        fprintf(f, "{\n");
        fprintf(f, "    switch (value) {\n");
        for (int j = 0; j < enums[i].valueslen; j++) {
            sprintf(decl_full_name, "%s%s", enums[i].prefix, enums[i].values[j].name);
            if (strlen(enums[i].values[j].label) == 0) {
                fprintf(f, "        case (%s): return \"%s\";\n", decl_full_name, decl_full_name);
            } else {
                fprintf(f, "        case (%s): return \"%s\";\n", decl_full_name, enums[i].values[j].label);
            }
        }
        fprintf(f, "        default: return \"UNDEFINED\";\n");
        fprintf(f, "    }\n");
        fprintf(f, "}\n");

        if (i < enumslen - 1)
            fprintf(f, "\n");
    }

    fclose(f);
    return 0;
}

int write_header(char* dir)
{
    char path[MAX_STRLEN];
    sprintf(path, "%s/%s", dir, hfile);

    FILE* f = fopen(path, "w");
    if (f == NULL) return 1;

    fprintf(f, "// GENERATED BY CENUM\n\n");
    fprintf(f, "#ifndef %s\n", hfile_ifndef);
    fprintf(f, "#define %s\n\n", hfile_ifndef);

    for (int i = 0; i < enumslen; i++) {
        fprintf(f, "typedef enum {\n");
        fprintf(f, "    %sBEGIN = 0,\n", enums[i].prefix);
        for (int j = 0; j < enums[i].valueslen; j++)
            fprintf(f, "    %s%s,\n", enums[i].prefix, enums[i].values[j].name);
        fprintf(f, "    %sEND\n", enums[i].prefix);
        fprintf(f, "} %s;\n\n", enums[i].name);
        fprintf(f, "char* toString_%s(%s value);\n\n", enums[i].name, enums[i].name);
    }

    fprintf(f, "#endif // %s\n", hfile_ifndef);

    fclose(f);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 3) return 1;

    size_t size = 8000000; // 8MB
    pool = malloc(size);
    memset(pool, 0, size);
    ptr = pool;

    enums = allocate(sizeof(Enum) * MAX_ENUMS);

    int retval = read_enum_file(argv[1]);
    if (!strlen(cfile) || !strlen(hfile) || retval) goto cleanup;

    retval = write_header(argv[2]);
    if (retval) goto cleanup;

    retval = write_implementation(argv[2]);
    if (retval) goto cleanup;

cleanup:
    free(pool);
    return retval;
}
