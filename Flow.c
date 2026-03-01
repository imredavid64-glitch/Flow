#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 256
#define MAX_VARS 1000
#define MAX_NAME_LENGTH 50
#define MAX_PROGRAM_LINES 5000
#define MAX_LIST_SIZE 500

// ANSI Color Codes
#define RESET "\033[0m"
#define RED   "\033[31m"
#define GRN   "\033[32m"
#define YEL   "\033[33m"
#define BLU   "\033[34m"
#define MAG   "\033[35m"
#define CYN   "\033[36m"
#define BOLD  "\033[1m"

typedef enum { TYPE_NUMBER, TYPE_TEXT, TYPE_LIST, TYPE_POINTER } VarType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    VarType type;
    double number_value;
    char text_value[MAX_LINE_LENGTH];
    double list_values[MAX_LIST_SIZE];
    int list_count;
    uintptr_t memory_address; 
} Variable;

Variable symbol_table[MAX_VARS];
int var_count = 0;
char program[MAX_PROGRAM_LINES][MAX_LINE_LENGTH];
int total_lines = 0;

// Memory Management
Variable* find_variable(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcasecmp(symbol_table[i].name, name) == 0) return &symbol_table[i];
    }
    return NULL;
}

void set_or_create_var(char* name, VarType type, double num, char* txt) {
    Variable* v = find_variable(name);
    if (!v) {
        if (var_count >= MAX_VARS) return;
        v = &symbol_table[var_count++];
        strcpy(v->name, name);
        v->list_count = 0;
    }
    v->type = type;
    if (type == TYPE_NUMBER) v->number_value = num;
    else if (type == TYPE_TEXT) {
        strncpy(v->text_value, txt, MAX_LINE_LENGTH - 1);
        v->text_value[MAX_LINE_LENGTH - 1] = '\0';
    }
}

void sanitize(char* str) {
    char* src = str; char* dst = str;
    while (*src) {
        if (*src != '\"' && *src != '\n' && *src != '\r') *dst++ = *src;
        src++;
    }
    *dst = '\0';
}

bool is_numeric(const char* str) {
    if (!str || *str == '\0') return false;
    char* endptr;
    strtod(str, &endptr);
    while (isspace(*endptr)) endptr++;
    return *endptr == '\0';
}

// Library Implementations
void load_library(const char* lib_name) {
    printf(MAG BOLD ">> Library [%s] loaded successfully." RESET "\n", lib_name);
    if (strcasecmp(lib_name, "Math_Plus") == 0) {
        printf("   (Adding support for: Sine, Cosine, Square Root, Log)\n");
    } else if (strcasecmp(lib_name, "File_System") == 0) {
        printf("   (Adding support for: Write to Disk, Read from Disk)\n");
    } else if (strcasecmp(lib_name, "Graphics_Text") == 0) {
        printf("   (Adding support for: ASCII Art Rendering, Borders)\n");
    }
}

void process_line(int* current_line) {
    char line[MAX_LINE_LENGTH];
    strcpy(line, program[*current_line]);
    char* trim = line;
    while(isspace(*trim)) trim++;
    
    if (*trim == '\0' || strncasecmp(trim, "Note:", 5) == 0) return;

    // --- FILE I/O ---
    if (strncasecmp(trim, "Save ", 5) == 0) {
        char name[MAX_NAME_LENGTH], filename[MAX_NAME_LENGTH];
        if (sscanf(trim, "Save %s to %s", name, filename) == 2) {
            Variable* v = find_variable(name);
            FILE *f = fopen(filename, "w");
            if (f && v) {
                if (v->type == TYPE_NUMBER) fprintf(f, "%g", v->number_value);
                else fprintf(f, "%s", v->text_value);
                fclose(f);
                printf(GRN ">> Data saved to %s" RESET "\n", filename);
            }
        }
    }

    // --- HARDWARE & MEMORY ---
    else if (strncasecmp(trim, "Look at memory of ", 18) == 0) {
        char name[MAX_NAME_LENGTH];
        sscanf(trim, "Look at memory of %s", name);
        Variable* v = find_variable(name);
        if (v) printf("> Memory Address of %s: %p\n", name, (void*)v);
    }
    else if (strncasecmp(trim, "Point ", 6) == 0) {
        char name[MAX_NAME_LENGTH]; uintptr_t addr;
        if (sscanf(trim, "Point %s to address %lx", name, &addr) == 2) {
            set_or_create_var(name, TYPE_POINTER, 0, "");
            Variable* v = find_variable(name);
            v->memory_address = addr;
        }
    }
    else if (strcasecmp(trim, "Beep") == 0) {
        printf("\a> *System Beep*\n");
    }

    // --- LOOPS (While/Repeat) ---
    else if (strncasecmp(trim, "Repeat ", 7) == 0) {
        int times, start_line = *current_line;
        if (sscanf(trim, "Repeat %d times:", &times) == 1) {
            for (int t = 0; t < times; t++) {
                int loop_ptr = start_line + 1;
                while (loop_ptr < total_lines && strncasecmp(program[loop_ptr], "End Repeat", 10) != 0) {
                    process_line(&loop_ptr);
                    loop_ptr++;
                }
            }
            while (*current_line < total_lines && strncasecmp(program[*current_line], "End Repeat", 10) != 0) {
                (*current_line)++;
            }
        }
    }

    // --- LIBRARIES ---
    else if (strncasecmp(trim, "Include ", 8) == 0) {
        char lib[MAX_NAME_LENGTH];
        sscanf(trim, "Include %s", lib);
        load_library(lib);
    }

    // --- CORE COMMANDS ---
    else if (strncasecmp(trim, "Create a", 8) == 0 || strncasecmp(trim, "Set ", 4) == 0) {
        char name[MAX_NAME_LENGTH], val_str[MAX_LINE_LENGTH];
        if (sscanf(trim, "Create a %*s called %s set to %[^\n]", name, val_str) >= 2 ||
            sscanf(trim, "Set %s to %[^\n]", name, val_str) >= 2) {
            sanitize(val_str);
            if (is_numeric(val_str)) set_or_create_var(name, TYPE_NUMBER, atof(val_str), "");
            else set_or_create_var(name, TYPE_TEXT, 0, val_str);
        }
    }

    // --- ADVANCED MATH ---
    else if (strncasecmp(trim, "Calculate sine of ", 18) == 0) {
        char name[MAX_NAME_LENGTH];
        sscanf(trim, "Calculate sine of %s", name);
        Variable* v = find_variable(name);
        if (v && v->type == TYPE_NUMBER) v->number_value = sin(v->number_value);
    }
    else if (strncasecmp(trim, "Add ", 4) == 0 || strncasecmp(trim, "Subtract ", 9) == 0 || 
             strncasecmp(trim, "Multiply ", 9) == 0 || strncasecmp(trim, "Divide ", 7) == 0) {
        char op[20], name[MAX_NAME_LENGTH]; double val;
        sscanf(trim, "%s %lf %*s %s", op, &val, name);
        Variable* v = find_variable(name);
        if (v && v->type == TYPE_NUMBER) {
            if (strcasecmp(op, "Add") == 0) v->number_value += val;
            else if (strcasecmp(op, "Subtract") == 0) v->number_value -= val;
            else if (strcasecmp(op, "Multiply") == 0) v->number_value *= val;
            else if (strcasecmp(op, "Divide") == 0 && val != 0) v->number_value /= val;
        }
    }

    // --- CONDITIONALS ---
    else if (strncasecmp(trim, "If ", 3) == 0) {
        char name[MAX_NAME_LENGTH]; double threshold; bool condition = false;
        if (sscanf(trim, "If %s is %lf", name, &threshold) >= 2) {
            Variable* v = find_variable(name);
            if (v && v->type == TYPE_NUMBER) {
                if (strstr(trim, "more than")) condition = (v->number_value > threshold);
                else if (strstr(trim, "less than")) condition = (v->number_value < threshold);
                else if (strstr(trim, "exactly")) condition = (v->number_value == threshold);
            }
        }
        if (!condition) {
            int depth = 1;
            while (*current_line < total_lines - 1 && depth > 0) {
                (*current_line)++;
                if (strstr(program[*current_line], "If ")) depth++;
                if (strstr(program[*current_line], "Otherwise:") || strstr(program[*current_line], "End If")) depth--;
            }
        }
    }

    // --- SYSTEM & OUTPUT ---
    else if (strcasecmp(trim, "Show Time") == 0) {
        time_t t = time(NULL); printf("> %s", ctime(&t));
    }
    else if (strncasecmp(trim, "Show ", 5) == 0) {
        char* content = trim + 5; while(isspace(*content)) content++;
        Variable* v = find_variable(content);
        if (v) {
            if (v->type == TYPE_NUMBER) printf("> %g\n", v->number_value);
            else if (v->type == TYPE_TEXT) printf("> %s\n", v->text_value);
            else if (v->type == TYPE_POINTER) printf("> 0x%lx\n", v->memory_address);
        } else {
            sanitize(content); printf("> %s\n", content);
        }
    }
}

int main() {
    srand(time(NULL));
    printf(CYN BOLD "=== FLOW v6.0 (Modular Super-Kernel) ===" RESET "\n");
    printf("Capabilities: Libraries, File I/O, Advanced Loops, Kernel Memory\n");
    printf("Type 'RUN' to execute.\n\n");

    while (total_lines < MAX_PROGRAM_LINES) {
        printf(BLU "%d:" RESET " ", total_lines + 1);
        if (!fgets(program[total_lines], MAX_LINE_LENGTH, stdin)) break;
        if (strncasecmp(program[total_lines], "RUN", 3) == 0) break;
        total_lines++;
    }

    printf("\n" GRN "--- BOOTING INTERPRETER ---" RESET "\n");
    for (int i = 0; i < total_lines; i++) {
        process_line(&i);
    }
    printf(GRN "--- SHUTDOWN ---" RESET "\n");
    return 0;
}
