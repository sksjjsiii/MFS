/*
 * WebFast Compiler - Main Driver
 * Reads WebFast source code and generates C code
 * 
 * Usage: wf <input.wf> [-o output.c]
 */

#include "../include/webfast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read file contents
static char* read_file(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, size, fp);
    buffer[bytes_read] = '\0';
    
    fclose(fp);
    return buffer;
}

// Write file contents
static int write_file(const char* filename, const char* content) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot write to file '%s'\n", filename);
        return -1;
    }
    
    fputs(content, fp);
    fclose(fp);
    
    return 0;
}

int main(int argc, char** argv) {
    printf("\033[1;34m");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     WebFast Compiler v1.0.0            ║\n");
    printf("║     A Web Programming Language         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\033[0m\n");
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.wf> [-o output.c]\n", argv[0]);
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "  -o <file>    Output C file (default: output.c)\n");
        fprintf(stderr, "  -v           Verbose output\n");
        fprintf(stderr, "  -h           Show this help\n");
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = "output.c";
    int verbose = 0;
    
    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s <input.wf> [-o output.c]\n\n", argv[0]);
            printf("Options:\n");
            printf("  -o <file>    Output C file (default: output.c)\n");
            printf("  -v           Verbose output\n");
            printf("  -h           Show this help\n");
            return 0;
        }
    }
    
    printf("\033[1;32m[1/4]\033[0m Reading source file: %s\n", input_file);
    char* source = read_file(input_file);
    if (!source) {
        return 1;
    }
    
    if (verbose) {
        printf("  Source size: %zu bytes\n", strlen(source));
    }
    
    printf("\033[1;32m[2/4]\033[0m Tokenizing...\n");
    Token* tokens = lexer_tokenize(source);
    if (!tokens) {
        free(source);
        return 1;
    }
    
    if (verbose) {
        int token_count = 0;
        Token* t = tokens;
        while (t->type != TOK_EOF) {
            token_count++;
            t++;
        }
        printf("  Generated %d tokens\n", token_count);
    }
    
    printf("\033[1;32m[3/4]\033[0m Parsing...\n");
    ASTNode* ast = parser_parse(tokens);
    if (!ast) {
        lexer_free_tokens(tokens);
        free(source);
        return 1;
    }
    
    if (verbose) {
        printf("  AST built successfully\n");
        printf("\nAST Structure:\n");
        ast_print(ast, 0);
    }
    
    printf("\033[1;32m[4/4]\033[0m Generating C code...\n");
    char* code = codegen_generate(ast);
    if (!code) {
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 1;
    }
    
    if (verbose) {
        printf("  Generated %zu bytes of C code\n", strlen(code));
    }
    
    // Write output
    if (write_file(output_file, code) != 0) {
        codegen_free(code);
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 1;
    }
    
    printf("\n\033[1;32m✓ Compilation successful!\033[0m\n");
    printf("  Output: %s\n", output_file);
    printf("\nTo compile the generated code:\n");
    printf("  gcc %s webfast_runtime.c -o myapp -lpthread\n", output_file);
    printf("  ./myapp\n\n");
    
    // Cleanup
    codegen_free(code);
    ast_free(ast);
    lexer_free_tokens(tokens);
    free(source);
    
    return 0;
}
