/*
 * WebFast v3.0 Lexer - Extended Tokenizer
 */

#include "../include/webfast.h"
#include <stdarg.h>

void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\033[31mError:\033[0m ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

char* string_duplicate(const char* str) {
    if (!str) return NULL;
    char* dup = malloc(strlen(str) + 1);
    if (dup) strcpy(dup, str);
    return dup;
}

static TokenType lookup_keyword(const char* word) {
    // Core keywords
    if (strcmp(word, "config") == 0) return TOK_CONFIG;
    if (strcmp(word, "route") == 0) return TOK_ROUTE;
    if (strcmp(word, "GET") == 0) return TOK_GET;
    if (strcmp(word, "POST") == 0) return TOK_POST;
    if (strcmp(word, "PUT") == 0) return TOK_PUT;
    if (strcmp(word, "DELETE") == 0) return TOK_DELETE;
    if (strcmp(word, "PATCH") == 0) return TOK_PATCH;
    if (strcmp(word, "HEAD") == 0) return TOK_HEAD;
    if (strcmp(word, "OPTIONS") == 0) return TOK_OPTIONS;
    if (strcmp(word, "seo") == 0) return TOK_SEO;
    if (strcmp(word, "return") == 0) return TOK_RETURN;
    if (strcmp(word, "if") == 0) return TOK_IF;
    if (strcmp(word, "else") == 0) return TOK_ELSE;
    if (strcmp(word, "for") == 0) return TOK_FOR;
    if (strcmp(word, "in") == 0) return TOK_IN;
    if (strcmp(word, "while") == 0) return TOK_WHILE;
    if (strcmp(word, "do") == 0) return TOK_DO;
    if (strcmp(word, "break") == 0) return TOK_BREAK;
    if (strcmp(word, "continue") == 0) return TOK_CONTINUE;
    if (strcmp(word, "null") == 0) return TOK_NULL;
    if (strcmp(word, "true") == 0) return TOK_TRUE;
    if (strcmp(word, "false") == 0) return TOK_FALSE;
    if (strcmp(word, "now") == 0) return TOK_NOW;
    if (strcmp(word, "query") == 0) return TOK_QUERY;
    if (strcmp(word, "body") == 0) return TOK_BODY;
    if (strcmp(word, "headers") == 0) return TOK_HEADERS;
    if (strcmp(word, "params") == 0) return TOK_PARAMS;
    
    // Function keywords
    if (strcmp(word, "function") == 0) return TOK_FUNCTION;
    if (strcmp(word, "let") == 0) return TOK_LET;
    if (strcmp(word, "const") == 0) return TOK_CONST;
    if (strcmp(word, "var") == 0) return TOK_VAR;
    
    // OOP keywords
    if (strcmp(word, "type") == 0) return TOK_TYPE;
    if (strcmp(word, "interface") == 0) return TOK_INTERFACE;
    if (strcmp(word, "struct") == 0) return TOK_STRUCT;
    if (strcmp(word, "class") == 0) return TOK_CLASS;
    if (strcmp(word, "public") == 0) return TOK_PUBLIC;
    if (strcmp(word, "private") == 0) return TOK_PRIVATE;
    if (strcmp(word, "protected") == 0) return TOK_PROTECTED;
    if (strcmp(word, "static") == 0) return TOK_STATIC;
    if (strcmp(word, "extends") == 0) return TOK_EXTENDS;
    if (strcmp(word, "implements") == 0) return TOK_IMPLEMENTS;
    if (strcmp(word, "new") == 0) return TOK_NEW;
    if (strcmp(word, "this") == 0) return TOK_THIS;
    if (strcmp(word, "self") == 0) return TOK_SELF;
    if (strcmp(word, "super") == 0) return TOK_SUPER;
    
    // Async keywords
    if (strcmp(word, "async") == 0) return TOK_ASYNC;
    if (strcmp(word, "await") == 0) return TOK_AWAIT;
    
    // Error handling
    if (strcmp(word, "try") == 0) return TOK_TRY;
    if (strcmp(word, "catch") == 0) return TOK_CATCH;
    if (strcmp(word, "finally") == 0) return TOK_FINALLY;
    if (strcmp(word, "throw") == 0) return TOK_THROW;
    if (strcmp(word, "error") == 0) return TOK_ERROR;
    
    // Module keywords
    if (strcmp(word, "import") == 0) return TOK_IMPORT;
    if (strcmp(word, "export") == 0) return TOK_EXPORT;
    if (strcmp(word, "use") == 0) return TOK_USE;
    
    // Middleware
    if (strcmp(word, "middleware") == 0) return TOK_MIDDLEWARE;
    if (strcmp(word, "before") == 0) return TOK_BEFORE;
    if (strcmp(word, "after") == 0) return TOK_AFTER;
    
    return TOK_IDENTIFIER;
}

Token* lexer_tokenize(const char* source) {
    if (!source) return NULL;
    
    size_t capacity = 1024;
    size_t count = 0;
    Token* tokens = malloc(capacity * sizeof(Token));
    
    if (!tokens) {
        error("Memory allocation failed");
        return NULL;
    }
    
    int line = 1;
    int col = 1;
    size_t pos = 0;
    size_t len = strlen(source);
    
    while (pos < len) {
        char c = source[pos];
        
        // Skip whitespace
        if (c == ' ' || c == '\t' || c == '\r') {
            col++;
            pos++;
            continue;
        }
        
        // Newline
        if (c == '\n') {
            line++;
            col = 1;
            pos++;
            continue;
        }
        
        // Comments
        if (c == '#') {
            while (pos < len && source[pos] != '\n') {
                pos++;
            }
            continue;
        }
        
        // Multi-line comments /* ... */
        if (c == '/' && pos + 1 < len && source[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < len && !(source[pos] == '*' && source[pos + 1] == '/')) {
                if (source[pos] == '\n') {
                    line++;
                    col = 1;
                }
                pos++;
            }
            pos += 2;
            continue;
        }
        
        // Strings
        if (c == '"') {
            int start_col = col;
            pos++;
            col++;
            
            size_t str_start = pos;
            while (pos < len && source[pos] != '"') {
                if (source[pos] == '\\' && pos + 1 < len) {
                    pos += 2;
                    col += 2;
                } else {
                    pos++;
                    col++;
                }
            }
            
            if (pos >= len) {
                error("Unterminated string at line %d", line);
                free(tokens);
                return NULL;
            }
            
            size_t str_len = pos - str_start;
            char* str_val = malloc(str_len + 1);
            strncpy(str_val, source + str_start, str_len);
            str_val[str_len] = '\0';
            
            if (count >= capacity) {
                capacity *= 2;
                tokens = realloc(tokens, capacity * sizeof(Token));
            }
            
            tokens[count].type = TOK_STRING;
            tokens[count].value = str_val;
            tokens[count].line = line;
            tokens[count].column = start_col;
            count++;
            
            pos++;
            col++;
            continue;
        }
        
        // Template strings `...`
        if (c == '`') {
            int start_col = col;
            pos++;
            col++;
            
            size_t str_start = pos;
            while (pos < len && source[pos] != '`') {
                if (source[pos] == '\\' && pos + 1 < len) {
                    pos += 2;
                    col += 2;
                } else {
                    pos++;
                    col++;
                }
            }
            
            if (pos >= len) {
                error("Unterminated template string at line %d", line);
                free(tokens);
                return NULL;
            }
            
            size_t str_len = pos - str_start;
            char* str_val = malloc(str_len + 1);
            strncpy(str_val, source + str_start, str_len);
            str_val[str_len] = '\0';
            
            if (count >= capacity) {
                capacity *= 2;
                tokens = realloc(tokens, capacity * sizeof(Token));
            }
            
            tokens[count].type = TOK_TEMPLATE_STRING;
            tokens[count].value = str_val;
            tokens[count].line = line;
            tokens[count].column = start_col;
            count++;
            
            pos++;
            col++;
            continue;
        }
        
        // Numbers
        if (isdigit(c)) {
            int start_col = col;
            size_t num_start = pos;
            bool is_float = false;
            
            while (pos < len && (isdigit(source[pos]) || source[pos] == '.')) {
                if (source[pos] == '.') {
                    if (is_float) break;
                    is_float = true;
                }
                pos++;
                col++;
            }
            
            size_t num_len = pos - num_start;
            char* num_val = malloc(num_len + 1);
            strncpy(num_val, source + num_start, num_len);
            num_val[num_len] = '\0';
            
            if (count >= capacity) {
                capacity *= 2;
                tokens = realloc(tokens, capacity * sizeof(Token));
            }
            
            tokens[count].type = is_float ? TOK_FLOAT : TOK_INTEGER;
            tokens[count].value = num_val;
            tokens[count].line = line;
            tokens[count].column = start_col;
            count++;
            continue;
        }
        
        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            int start_col = col;
            size_t id_start = pos;
            
            while (pos < len && (isalnum(source[pos]) || source[pos] == '_')) {
                pos++;
                col++;
            }
            
            size_t id_len = pos - id_start;
            char* id_val = malloc(id_len + 1);
            strncpy(id_val, source + id_start, id_len);
            id_val[id_len] = '\0';
            
            if (count >= capacity) {
                capacity *= 2;
                tokens = realloc(tokens, capacity * sizeof(Token));
            }
            
            TokenType type = lookup_keyword(id_val);
            tokens[count].type = type;
            tokens[count].value = id_val;
            tokens[count].line = line;
            tokens[count].column = start_col;
            count++;
            continue;
        }
        
        // Operators and delimiters
        int start_col = col;
        TokenType type = TOK_ERROR;
        char* val = malloc(4);
        
        switch (c) {
            case '{': type = TOK_LBRACE; strcpy(val, "{"); break;
            case '}': type = TOK_RBRACE; strcpy(val, "}"); break;
            case '[': type = TOK_LBRACKET; strcpy(val, "["); break;
            case ']': type = TOK_RBRACKET; strcpy(val, "]"); break;
            case '(': type = TOK_LPAREN; strcpy(val, "("); break;
            case ')': type = TOK_RPAREN; strcpy(val, ")"); break;
            case ':': type = TOK_COLON; strcpy(val, ":"); break;
            case ';': type = TOK_SEMICOLON; strcpy(val, ";"); break;
            case ',': type = TOK_COMMA; strcpy(val, ","); break;
            case '@': type = TOK_AT; strcpy(val, "@"); break;
            case '#': type = TOK_HASH; strcpy(val, "#"); break;
            case '?':
                if (pos + 1 < len && source[pos + 1] == '?') {
                    type = TOK_NULL_COALESCE;
                    strcpy(val, "??");
                    pos++;
                    col++;
                } else if (pos + 1 < len && source[pos + 1] == '.') {
                    type = TOK_OPTIONAL_CHAIN;
                    strcpy(val, "?.");
                    pos++;
                    col++;
                } else {
                    type = TOK_QUESTION;
                    strcpy(val, "?");
                }
                break;
            case '.':
                if (pos + 2 < len && source[pos + 1] == '.' && source[pos + 2] == '.') {
                    type = TOK_SPREAD;
                    strcpy(val, "...");
                    pos += 2;
                    col += 2;
                } else {
                    type = TOK_DOT;
                    strcpy(val, ".");
                }
                break;
            case '+':
                if (pos + 1 < len && source[pos + 1] == '+') {
                    type = TOK_INCREMENT;
                    strcpy(val, "++");
                    pos++;
                    col++;
                } else if (pos + 1 < len && source[pos + 1] == '=') {
                    type = TOK_PLUS_ASSIGN;
                    strcpy(val, "+=");
                    pos++;
                    col++;
                } else {
                    type = TOK_PLUS;
                    strcpy(val, "+");
                }
                break;
            case '-':
                if (pos + 1 < len && source[pos + 1] == '-') {
                    type = TOK_DECREMENT;
                    strcpy(val, "--");
                    pos++;
                    col++;
                } else if (pos + 1 < len && source[pos + 1] == '=') {
                    type = TOK_MINUS_ASSIGN;
                    strcpy(val, "-=");
                    pos++;
                    col++;
                } else {
                    type = TOK_MINUS;
                    strcpy(val, "-");
                }
                break;
            case '*':
                if (pos + 1 < len && source[pos + 1] == '*') {
                    type = TOK_POWER;
                    strcpy(val, "**");
                    pos++;
                    col++;
                } else if (pos + 1 < len && source[pos + 1] == '=') {
                    type = TOK_STAR_ASSIGN;
                    strcpy(val, "*=");
                    pos++;
                    col++;
                } else {
                    type = TOK_MULTIPLY;
                    strcpy(val, "*");
                }
                break;
            case '/':
                if (pos + 1 < len && source[pos + 1] == '=') {
                    type = TOK_SLASH_ASSIGN;
                    strcpy(val, "/=");
                    pos++;
                    col++;
                } else {
                    type = TOK_DIVIDE;
                    strcpy(val, "/");
                }
                break;
            case '%': type = TOK_MODULO; strcpy(val, "%"); break;
            case '<':
                if (pos + 1 < len && source[pos + 1] == '=') {
                    type = TOK_LESS_EQ;
                    strcpy(val, "<=");
                    pos++;
                    col++;
                } else if (pos + 1 < len && source[pos + 1] == '<') {
                    type = TOK_SHIFT_LEFT;
                    strcpy(val, "<<");
                    pos++;
                    col++;
                } else {
                    type = TOK_LESS;
                    strcpy(val, "<");
                }
                break;
            case '>':
                if (pos + 1 < len && source[pos + 1] == '=') {
                    type = TOK_GREATER_EQ;
                    strcpy(val, ">=");
                    pos++;
                    col++;
                } else if (pos + 1 < len && source[pos + 1] == '>') {
                    type = TOK_SHIFT_RIGHT;
                    strcpy(val, ">>");
                    pos++;
                    col++;
                } else {
                    type = TOK_GREATER;
                    strcpy(val, ">");
                }
                break;
            case '=':
                if (pos + 1 < len && source[pos + 1] == '=') {
                    if (pos + 2 < len && source[pos + 2] == '=') {
                        type = TOK_STRICT_EQUAL;
                        strcpy(val, "===");
                        pos += 2;
                        col += 2;
                    } else {
                        type = TOK_EQUAL;
                        strcpy(val, "==");
                        pos++;
                        col++;
                    }
                } else if (pos + 1 < len && source[pos + 1] == '>') {
                    type = TOK_ARROW;
                    strcpy(val, "=>");
                    pos++;
                    col++;
                } else {
                    type = TOK_ASSIGN;
                    strcpy(val, "=");
                }
                break;
            case '!':
                if (pos + 1 < len && source[pos + 1] == '=') {
                    if (pos + 2 < len && source[pos + 2] == '=') {
                        type = TOK_STRICT_NOT_EQUAL;
                        strcpy(val, "!==");
                        pos += 2;
                        col += 2;
                    } else {
                        type = TOK_NOT_EQUAL;
                        strcpy(val, "!=");
                        pos++;
                        col++;
                    }
                } else {
                    type = TOK_NOT;
                    strcpy(val, "!");
                }
                break;
            case '&':
                if (pos + 1 < len && source[pos + 1] == '&') {
                    type = TOK_AND;
                    strcpy(val, "&&");
                    pos++;
                    col++;
                } else {
                    type = TOK_BIT_AND;
                    strcpy(val, "&");
                }
                break;
            case '|':
                if (pos + 1 < len && source[pos + 1] == '|') {
                    type = TOK_OR;
                    strcpy(val, "||");
                    pos++;
                    col++;
                } else {
                    type = TOK_BIT_OR;
                    strcpy(val, "|");
                }
                break;
            case '^': type = TOK_BIT_XOR; strcpy(val, "^"); break;
            case '~': type = TOK_BIT_NOT; strcpy(val, "~"); break;
            default:
                error("Unexpected character '%c' at line %d, column %d", c, line, col);
                free(val);
                free(tokens);
                return NULL;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(Token));
        }
        
        tokens[count].type = type;
        tokens[count].value = val;
        tokens[count].line = line;
        tokens[count].column = start_col;
        count++;
        
        pos++;
        col++;
    }
    
    // Add EOF token
    if (count >= capacity) {
        capacity++;
        tokens = realloc(tokens, capacity * sizeof(Token));
    }
    
    tokens[count].type = TOK_EOF;
    tokens[count].value = string_duplicate("EOF");
    tokens[count].line = line;
    tokens[count].column = col;
    count++;
    
    return tokens;
}

void lexer_free_tokens(Token* tokens) {
    if (!tokens) return;
    
    Token* current = tokens;
    while (current->type != TOK_EOF) {
        free(current->value);
        current++;
    }
    free(current->value);
    free(tokens);
}
