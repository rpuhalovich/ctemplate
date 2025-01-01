#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STRLEN 512
#define MAX_ENUMS 512
#define MAX_ENUM_VALUES 512

unsigned char* pool;
unsigned char* ptr;

char cfile[MAX_STRLEN];
char hfile[MAX_STRLEN];
char hfile_ifndef[MAX_STRLEN];

int decl_len = 0;
char decl[MAX_ENUMS][MAX_STRLEN];
char* decl_values[MAX_ENUM_VALUES][MAX_ENUM_VALUES];
int decl_values_lens[MAX_ENUM_VALUES];

char prefix[MAX_ENUMS][MAX_ENUM_VALUES];
char* optional_values[MAX_ENUM_VALUES][MAX_STRLEN];

void read_enum_file(char* path)
{
    FILE* f = fopen(path, "r");
    if (f == NULL)
        exit(1);

    int cur_value_index = 0;

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, f)) > 0) {
        if (!linelen)
            continue;

        int i = 0;
        for (; line[i] != ' ' && i < linelen; i++);

        if (strncmp("CFILE", line, strlen("CFILE")) == 0)
            memcpy(cfile, (char*)(line + i + 1), linelen - i - 2);

        if (strncmp("HFILE", line, strlen("HFILE")) == 0) {
            memcpy(hfile, (char*)(line + i + 1), linelen - i - 2);

            memcpy(hfile_ifndef, (char*)(line + i + 1), linelen - i - 2);
            for (int j = 0; hfile_ifndef[j] != '\0'; j++) {
                if (hfile_ifndef[j] == '.')
                    hfile_ifndef[j] = '_';
                else
                    hfile_ifndef[j] = toupper(hfile_ifndef[j]);
            }
        }

        if (strncmp("ENUM", line, strlen("ENUM")) == 0) {
            int j = i + 1;
            for (; line[j] != ' ' && j < linelen; j++);

            memcpy(decl[decl_len], (char*)(line + i + 1), j - i - 1);
            memcpy(prefix[decl_len], (char*)(line + j + 1), linelen - j - 2);

            decl_len++;
            cur_value_index = 0;
        }

        if (strncmp("VALUE", line, strlen("VALUE")) == 0) {
            int j = i + 1;
            for (; line[j] != ' ' && j < linelen; j++);

            int cur_decl_index = decl_len - 1;
            if (j == linelen) {
                memcpy(decl_values[cur_decl_index][cur_value_index], (char*)(line + i + 1), linelen - i - 2);
            } else {
                memcpy(decl_values[cur_decl_index][cur_value_index], (char*)(line + i + 1), j - i - 1);
                memcpy(optional_values[cur_decl_index][cur_value_index], (char*)(line + j + 1), linelen - j - 2);
            }

            decl_values_lens[cur_decl_index]++;
            cur_value_index++;
        }
    }

    free(line);
    fclose(f);
}

void write_implementation(char* dir)
{
    char path[MAX_STRLEN];
    sprintf(path, "%s/%s", dir, cfile);

    FILE* f = fopen(path, "w");
    if (f == NULL)
        exit(1);

    fprintf(f, "// GENERATED BY CENUM\n\n");
    fprintf(f, "#include \"%s\"\n\n", hfile);

    char decl_full_name[MAX_STRLEN];
    for (int i = 0; i < decl_len; i++) {
        fprintf(f, "char* toString_%s(%s value)\n", decl[i], decl[i]);
        fprintf(f, "{\n");
        fprintf(f, "    switch (value) {\n");
        for (int j = 0; j < decl_values_lens[i]; j++) {
            if (strlen(optional_values[i][j]) == 0) {
                sprintf(decl_full_name, "%s%s", prefix[i], decl_values[i][j]);
                fprintf(f, "        case (%s): return \"%s\";\n", decl_full_name, decl_full_name);
            } else {
                sprintf(decl_full_name, "%s%s", prefix[i], decl_values[i][j]);
                fprintf(f, "        case (%s): return \"%s\";\n", decl_full_name, optional_values[i][j]);
            }
        }
        fprintf(f, "        default: return \"UNDEFINED\";\n");
        fprintf(f, "    }\n");
        fprintf(f, "}\n");

        if (i < decl_len - 1)
            fprintf(f, "\n");
    }

    fclose(f);
}

void write_header(char* dir)
{
    char path[MAX_STRLEN];
    sprintf(path, "%s/%s", dir, hfile);

    FILE* f = fopen(path, "w");
    if (f == NULL)
        exit(1);

    fprintf(f, "// GENERATED BY CENUM\n\n");
    fprintf(f, "#ifndef %s\n", hfile_ifndef);
    fprintf(f, "#define %s\n\n", hfile_ifndef);

    for (int i = 0; i < decl_len; i++) {
        fprintf(f, "typedef enum {\n");
        fprintf(f, "    %sBEGIN = 0,\n", prefix[i]);
        for (int j = 0; j < decl_values_lens[i]; j++)
            fprintf(f, "    %s%s,\n", prefix[i], decl_values[i][j]);
        fprintf(f, "    %sEND\n", prefix[i]);
        fprintf(f, "} %s;\n\n", decl[i]);

        fprintf(f, "char* toString_%s(%s value);\n\n", decl[i], decl[i]);
    }

    fprintf(f, "#endif // %s\n", hfile_ifndef);
    fclose(f);
}

int main(int argc, char** argv)
{
    if (argc != 3)
        return 1;

    size_t size = sizeof(unsigned char) * MAX_ENUM_VALUES * MAX_STRLEN * 2.0f;
    pool = malloc(size);
    memset(pool, 0, size);
    ptr = pool;

    for (int i = 0; i < MAX_ENUMS; i++) {
        for (int j = 0; j < MAX_ENUM_VALUES; j++) {
            decl_values[i][j] = (char*)ptr;
            ptr += sizeof(char) * MAX_STRLEN;

            optional_values[i][j] = (char*)ptr;
            ptr += sizeof(char) * MAX_STRLEN;
        }
    }

    read_enum_file(argv[1]);
    if (!strlen(cfile) || !strlen(hfile))
        return 2;

    write_header(argv[2]);
    write_implementation(argv[2]);

    free(pool);

    return 0;
}
