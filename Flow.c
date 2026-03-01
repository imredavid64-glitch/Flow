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
#define MAX_VARS 2000
#define MAX_NAME_LENGTH 64
#define MAX_PROGRAM_LINES 10000
#define MAX_LIST_SIZE 1000
#define MAX_FUNCTIONS 100

// ANSI Color Codes
#define RESET "\033[0m"
#define RED   "\033[31m"
#define GRN   "\033[32m"
#define YEL   "\033[33m"
#define BLU   "\033[34m"
#define MAG   "\033[35m"
#define CYN   "\033[36m"
#define BOLD  "\033[1m"

typedef enum { TYPE_NUMBER, TYPE_TEXT, TYPE_LIST, TYPE_POINTER, TYPE_VOID } VarType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    VarType type;
    double number_value;
    char text_value[MAX_LINE_LENGTH];
    double list_values[MAX_LIST_SIZE];
    int list_count;
    uintptr_t memory_address; 
} Variable;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int start_line;
} Function;

Variable symbol_table[MAX_VARS];
Function function_table[MAX_FUNCTIONS];
int var_count = 0;
int func_count = 0;
char program[MAX_PROGRAM_LINES][MAX_LINE_LENGTH];
int total_lines = 0;

// Utility Helpers
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

void load_library(const char* lib_name) {
    printf(MAG BOLD ">> Library [%s] integrated into kernel." RESET "\n", lib_name);
}

// Global process_line declaration for recursion
void process_line(int* current_line);

void process_line(int* current_line) {
    char line[MAX_LINE_LENGTH];
    strcpy(line, program[*current_line]);
    char* trim = line;
    while(isspace(*trim)) trim++;
    
    if (*trim == '\0' || strncasecmp(trim, "Note:", 5) == 0 || strncmp(trim, "//", 2) == 0) return;

    // --- FUNCTION DEFINITION (Skip during linear execution) ---
    if (strncasecmp(trim, "Function ", 9) == 0) {
        char func_name[MAX_NAME_LENGTH];
        sscanf(trim, "Function %[^:]", func_name);
        bool found = false;
        for(int f=0; f<func_count; f++) if(strcmp(function_table[f].name, func_name) == 0) found = true;
        if(!found) {
            strcpy(function_table[func_count].name, func_name);
            function_table[func_count++].start_line = *current_line;
        }
        while (*current_line < total_lines - 1 && strncasecmp(program[*current_line], "End Function", 12) != 0) {
            (*current_line)++;
        }
        return;
    }

    // --- CALL FUNCTION ---
    if (strncasecmp(trim, "Call ", 5) == 0) {
        char func_name[MAX_NAME_LENGTH];
        sscanf(trim, "Call %s", func_name);
        for (int f = 0; f < func_count; f++) {
            if (strcasecmp(function_table[f].name, func_name) == 0) {
                int call_ptr = function_table[f].start_line + 1;
                while (call_ptr < total_lines && strncasecmp(program[call_ptr], "End Function", 12) != 0) {
                    process_line(&call_ptr);
                    call_ptr++;
                }
                return;
            }
        }
        printf(RED "!! Function %s not found." RESET "\n", func_name);
    }

    // --- SYMBOLIC ASSIGNMENT & MATH (X = 10, X += 5, X = X * 2) ---
    if (strchr(trim, '=') && !strstr(trim, "==") && !strstr(trim, "!=") && !strstr(trim, "<=") && !strstr(trim, ">=")) {
        char name[MAX_NAME_LENGTH], val_str[MAX_LINE_LENGTH];
        // Handle +=, -=, *=, /=
        if (strstr(trim, "+=")) {
            sscanf(trim, "%s += %lf", name, &val_str[0]);
            Variable* v = find_variable(name);
            if (v) v->number_value += atof(val_str);
            return;
        }
        if (sscanf(trim, "%s = %[^\n]", name, val_str) == 2) {
            sanitize(val_str);
            if (is_numeric(val_str)) set_or_create_var(name, TYPE_NUMBER, atof(val_str), "");
            else {
                Variable* v_src = find_variable(val_str);
                if (v_src) set_or_create_var(name, v_src->type, v_src->number_value, v_src->text_value);
                else set_or_create_var(name, TYPE_TEXT, 0, val_str);
            }
            return;
        }
    }

    // --- REPEAT LOOP ---
    if (strncasecmp(trim, "Repeat ", 7) == 0) {
        int times, start_line = *current_line;
        if (sscanf(trim, "Repeat %d times:", &times) == 1) {
            for (int t = 0; t < times; t++) {
                int loop_ptr = start_line + 1;
                while (loop_ptr < total_lines && strncasecmp(program[loop_ptr], "End Repeat", 10) != 0) {
                    process_line(&loop_ptr);
                    loop_ptr++;
                }
            }
            while (*current_line < total_lines && strncasecmp(program[*current_line], "End Repeat", 10) != 0) (*current_line)++;
        }
        return;
    }

    // --- CONDITIONALS (Hybrid) ---
    if (strncasecmp(trim, "If ", 3) == 0) {
        char left[MAX_NAME_LENGTH], op[10], right[MAX_NAME_LENGTH];
        bool condition = false;
        if (sscanf(trim, "If %s %s %s", left, op, right) >= 3) {
            Variable* v_l = find_variable(left);
            Variable* v_r = find_variable(right);
            double l_val = v_l ? v_l->number_value : (is_numeric(left) ? atof(left) : 0);
            double r_val = v_r ? v_r->number_value : (is_numeric(right) ? atof(right) : 0);
            
            if (strcmp(op, ">") == 0 || strstr(trim, "more than")) condition = (l_val > r_val);
            else if (strcmp(op, "<") == 0 || strstr(trim, "less than")) condition = (l_val < r_val);
            else if (strcmp(op, ">=") == 0 || strstr(trim, "at least")) condition = (l_val >= r_val);
            else if (strcmp(op, "<=") == 0 || strstr(trim, "at most")) condition = (l_val <= r_val);
            else if (strcmp(op, "==") == 0 || strstr(trim, "exactly")) condition = (l_val == r_val);
            else if (strcmp(op, "!=") == 0 || strstr(trim, "not")) condition = (l_val != r_val);
        }
        if (!condition) {
            int depth = 1;
            while (*current_line < total_lines - 1 && depth > 0) {
                (*current_line)++;
                if (strncasecmp(program[*current_line], "If ", 3) == 0) depth++;
                if (strstr(program[*current_line], "Otherwise:") || strstr(program[*current_line], "End If")) depth--;
            }
        }
        return;
    }

    // --- HARDWARE / SYSTEM ---
    if (strncasecmp(trim, "Look at memory of ", 18) == 0) {
        char name[MAX_NAME_LENGTH]; sscanf(trim, "Look at memory of %s", name);
        Variable* v = find_variable(name);
        if (v) printf(CYN "> Memory of %s: %p" RESET "\n", name, (void*)v);
    } 
    else if (strncasecmp(trim, "Randomize ", 10) == 0) {
        char name[MAX_NAME_LENGTH]; double min, max;
        if (sscanf(trim, "Randomize %s between %lf and %lf", name, &min, &max) == 3) {
            double r = min + (rand() / (RAND_MAX / (max - min)));
            set_or_create_var(name, TYPE_NUMBER, r, "");
        }
    }
    else if (strncasecmp(trim, "Color ", 6) == 0) {
        if (strstr(trim, "Red")) printf(RED);
        else if (strstr(trim, "Green")) printf(GRN);
        else if (strstr(trim, "Blue")) printf(BLU);
        else if (strstr(trim, "Yellow")) printf(YEL);
        else printf(RESET);
    }
    else if (strcasecmp(trim, "Beep") == 0) printf("\a");
    else if (strcasecmp(trim, "Show Time") == 0) {
        time_t t = time(NULL); printf(YEL "> %s" RESET, ctime(&t));
    }
    else if (strncasecmp(trim, "Show ", 5) == 0) {
        char* content = trim + 5; while(isspace(*content)) content++;
        Variable* v = find_variable(content);
        if (v) {
            if (v->type == TYPE_NUMBER) printf("%g\n", v->number_value);
            else if (v->type == TYPE_TEXT) printf("%s\n", v->text_value);
            else if (v->type == TYPE_POINTER) printf("0x%lx\n", v->memory_address);
        } else {
            sanitize(content); printf("%s\n", content);
        }
    }
    else if (strncasecmp(trim, "Save ", 5) == 0) {
        char name[MAX_NAME_LENGTH], filename[MAX_NAME_LENGTH];
        if (sscanf(trim, "Save %s to %s", name, filename) == 2) {
            Variable* v = find_variable(name);
            FILE *f = fopen(filename, "w");
            if (f && v) {
                if (v->type == TYPE_NUMBER) fprintf(f, "%g", v->number_value);
                else fprintf(f, "%s", v->text_value);
                fclose(f);
                printf(GRN ">> %s written to disk." RESET "\n", filename);
            }
        }
    }
    else if (strncasecmp(trim, "Include ", 8) == 0) {
        char lib[MAX_NAME_LENGTH]; sscanf(trim, "Include %s", lib);
        load_library(lib);
    }
}

int main() {
    srand(time(NULL));
    printf(CYN BOLD "=== FLOW v8.0 HYBRID KERNEL ===" RESET "\n");
    printf("New: Functions, Symbolic Math, Randomization, Terminal Colors\n");
    printf("Ready for sequence. End code with 'RUN'.\n\n");

    while (total_lines < MAX_PROGRAM_LINES) {
        printf(BLU "%d:" RESET " ", total_lines + 1);
        if (!fgets(program[total_lines], MAX_LINE_LENGTH, stdin)) break;
        if (strncasecmp(program[total_lines], "RUN", 3) == 0) break;
        total_lines++;
    }

    printf("\n" GRN "--- EXECUTION START ---" RESET "\n");
    for (int i = 0; i < total_lines; i++) {
        process_line(&i);
    }
    printf(RESET GRN "--- EXECUTION COMPLETE ---" RESET "\n");
    return 0;
}
