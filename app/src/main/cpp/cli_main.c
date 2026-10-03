#define STANDALONE_CLI 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pocketpy.h"
#include "pocketpy_ext.h"

static char* read_file_string(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);
    return buf;
}

static void print_banner(void) {
    printf("PocketPy 2.0 (C11 Runtime with 160+ Native Extensions)\n");
    printf("Type 'exit()' or press Ctrl+C / Ctrl+D to quit.\n");
}

static void run_repl(void) {
    print_banner();
    char line[1024];
    while (1) {
        printf(">>> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        size_t len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
        }
        if (strcmp(line, "exit()") == 0 || strcmp(line, "quit()") == 0) break;
        if (strlen(line) == 0) continue;

        bool ok = py_exec(line, "<stdin>", EXEC_MODE, NULL);
        if (!ok) {
            char* err = py_formatexc();
            if (err) {
                fprintf(stderr, "%s\n", err);
                free(err);
            }
        }
    }
}

int main(int argc, char** argv) {
    py_initialize();
    register_all_pocketpy_extensions();

    int exit_code = 0;

    if (argc < 2) {
        run_repl();
    } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: pocketpy [options] [file.py] [args...]\n\n");
        printf("Options:\n");
        printf("  -c <cmd>       Execute Python command string\n");
        printf("  -v, --version  Show PocketPy version\n");
        printf("  -h, --help     Show this help message\n");
        printf("  [file.py]      Execute Python script file\n");
    } else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("PocketPy 2.0 (C11 with 160+ Extensions)\n");
    } else if (strcmp(argv[1], "-c") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Argument expected for -c\n");
            exit_code = 1;
        } else {
            bool ok = py_exec(argv[2], "<string>", EXEC_MODE, NULL);
            if (!ok) {
                char* err = py_formatexc();
                if (err) {
                    fprintf(stderr, "%s\n", err);
                    free(err);
                }
                exit_code = 1;
            }
        }
    } else {
        const char* script_path = argv[1];
        char* code = read_file_string(script_path);
        if (!code) {
            fprintf(stderr, "Error: Cannot open or read file '%s'\n", script_path);
            exit_code = 1;
        } else {
            bool ok = py_exec(code, script_path, EXEC_MODE, NULL);
            if (!ok) {
                char* err = py_formatexc();
                if (err) {
                    fprintf(stderr, "%s\n", err);
                    free(err);
                }
                exit_code = 1;
            }
            free(code);
        }
    }

    py_finalize();
    return exit_code;
}
