/*
 * WebFast v3.0 - Extended Header
 * Complete Web Programming Language
 */

#ifndef WEBFAST_H
#define WEBFAST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

// ============================================================================
// Token Types (Extended)
// ============================================================================

typedef enum {
    // Keywords
    TOK_CONFIG,
    TOK_ROUTE,
    TOK_GET,
    TOK_POST,
    TOK_PUT,
    TOK_DELETE,
    TOK_PATCH,
    TOK_HEAD,
    TOK_OPTIONS,
    TOK_SEO,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_IN,
    TOK_WHILE,
    TOK_DO,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_NULL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NOW,
    TOK_QUERY,
    TOK_BODY,
    TOK_HEADERS,
    TOK_PARAMS,
    TOK_FUNCTION,
    TOK_LET,
    TOK_CONST,
    TOK_VAR,
    TOK_TYPE,
    TOK_INTERFACE,
    TOK_STRUCT,
    TOK_CLASS,
    TOK_PUBLIC,
    TOK_PRIVATE,
    TOK_PROTECTED,
    TOK_STATIC,
    TOK_ASYNC,
    TOK_AWAIT,
    TOK_TRY,
    TOK_CATCH,
    TOK_FINALLY,
    TOK_THROW,
    TOK_IMPORT,
    TOK_EXPORT,
    TOK_USE,
    TOK_MIDDLEWARE,
    TOK_BEFORE,
    TOK_AFTER,
    TOK_ERROR,
    TOK_NEW,
    TOK_THIS,
    TOK_SELF,
    TOK_SUPER,
    TOK_EXTENDS,
    TOK_IMPLEMENTS,
    
    // Literals
    TOK_INTEGER,
    TOK_FLOAT,
    TOK_STRING,
    TOK_TEMPLATE_STRING,
    TOK_IDENTIFIER,
    TOK_REGEX,
    
    // Operators
    TOK_ASSIGN,      // =
    TOK_PLUS_ASSIGN, // +=
    TOK_MINUS_ASSIGN,// -=
    TOK_STAR_ASSIGN, // *=
    TOK_SLASH_ASSIGN,// /=
    TOK_EQUAL,       // ==
    TOK_STRICT_EQUAL,// ===
    TOK_NOT_EQUAL,   // !=
    TOK_STRICT_NOT_EQUAL, // !==
    TOK_LESS,        // <
    TOK_GREATER,     // >
    TOK_LESS_EQ,     // <=
    TOK_GREATER_EQ,  // >=
    TOK_PLUS,        // +
    TOK_MINUS,       // -
    TOK_MULTIPLY,    // *
    TOK_DIVIDE,      // /
    TOK_MODULO,      // %
    TOK_POWER,       // **
    TOK_INCREMENT,   // ++
    TOK_DECREMENT,   // --
    TOK_AND,         // &&
    TOK_OR,          // ||
    TOK_NOT,         // !
    TOK_BIT_AND,     // &
    TOK_BIT_OR,      // |
    TOK_BIT_XOR,     // ^
    TOK_BIT_NOT,     // ~
    TOK_SHIFT_LEFT,  // <<
    TOK_SHIFT_RIGHT, // >>
    TOK_ARROW,       // =>
    TOK_QUESTION,    // ?
    TOK_NULL_COALESCE, // ??
    TOK_OPTIONAL_CHAIN, // ?.
    TOK_SPREAD,      // ...
    
    // Delimiters
    TOK_LBRACE,      // {
    TOK_RBRACE,      // }
    TOK_LBRACKET,    // [
    TOK_RBRACKET,    // ]
    TOK_LPAREN,      // (
    TOK_RPAREN,      // )
    TOK_COLON,       // :
    TOK_SEMICOLON,   // ;
    TOK_COMMA,       // ,
    TOK_DOT,         // .
    TOK_AT,          // @
    TOK_HASH,        // #
    
    // Special
    TOK_EOF,
    TOK_NEWLINE,
    TOK_COMMENT,
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// ============================================================================
// AST Node Types (Extended)
// ============================================================================

typedef enum {
    NODE_PROGRAM,
    NODE_CONFIG,
    NODE_CONFIG_ENTRY,
    NODE_ROUTE,
    NODE_SEO_BLOCK,
    NODE_STATEMENT_LIST,
    NODE_VAR_DECL,
    NODE_VAR_ASSIGN,
    NODE_IF_STMT,
    NODE_FOR_STMT,
    NODE_WHILE_STMT,
    NODE_DO_WHILE_STMT,
    NODE_RETURN_STMT,
    NODE_BREAK_STMT,
    NODE_CONTINUE_STMT,
    NODE_FUNCTION_DECL,
    NODE_FUNCTION_CALL,
    NODE_ARROW_FUNCTION,
    NODE_CLASS_DECL,
    NODE_STRUCT_DECL,
    NODE_INTERFACE_DECL,
    NODE_METHOD_DECL,
    NODE_PROPERTY_DECL,
    NODE_TRY_CATCH,
    NODE_THROW_STMT,
    NODE_IMPORT_STMT,
    NODE_EXPORT_STMT,
    NODE_MIDDLEWARE,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_TERNARY_OP,
    NODE_NULL_COALESCE_OP,
    NODE_OPTIONAL_CHAIN,
    NODE_SPREAD_OP,
    NODE_IDENTIFIER,
    NODE_INT_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_STRING_LITERAL,
    NODE_TEMPLATE_STRING,
    NODE_BOOL_LITERAL,
    NODE_NULL_LITERAL,
    NODE_UNDEFINED_LITERAL,
    NODE_ARRAY,
    NODE_OBJECT,
    NODE_OBJECT_ENTRY,
    NODE_MEMBER_ACCESS,
    NODE_INDEX_ACCESS,
    NODE_TYPE_ANNOTATION,
    NODE_TYPE_ALIAS,
    NODE_NOW_EXPR,
    NODE_QUERY_EXPR,
    NODE_BODY_EXPR,
    NODE_HEADERS_EXPR,
    NODE_PARAMS_EXPR,
    NODE_ROUTE_PARAM,
    NODE_DECORATOR,
    NODE_ASYNC_EXPR,
    NODE_AWAIT_EXPR
} NodeType;

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct NodeList NodeList;

struct NodeList {
    ASTNode* node;
    struct NodeList* next;
};

struct ASTNode {
    NodeType type;
    
    union {
        // Program
        struct {
            NodeList* configs;
            NodeList* imports;
            NodeList* functions;
            NodeList* classes;
            NodeList* routes;
            NodeList* middlewares;
        } program;
        
        // Config
        struct {
            NodeList* entries;
        } config;
        
        // Config Entry
        struct {
            char* key;
            ASTNode* value;
        } config_entry;
        
        // Route
        struct {
            char* method;
            char* path;
            NodeList* params;
            NodeList* middlewares;
            NodeList* statements;
        } route;
        
        // Function Declaration
        struct {
            char* name;
            NodeList* params;
            ASTNode* return_type;
            NodeList* body;
            bool is_async;
        } function_decl;
        
        // Function Call
        struct {
            ASTNode* callee;
            NodeList* arguments;
        } function_call;
        
        // Variable Declaration
        struct {
            char* name;
            ASTNode* type_annotation;
            ASTNode* initializer;
            bool is_const;
        } var_decl;
        
        // Variable Assignment
        struct {
            ASTNode* target;
            ASTNode* value;
            char* operator;
        } var_assign;
        
        // If Statement
        struct {
            ASTNode* condition;
            NodeList* then_branch;
            NodeList* else_branch;
        } if_stmt;
        
        // For Statement
        struct {
            char* var_name;
            ASTNode* iterable;
            NodeList* body;
        } for_stmt;
        
        // While Statement
        struct {
            ASTNode* condition;
            NodeList* body;
        } while_stmt;
        
        // Return Statement
        struct {
            char* return_type;
            ASTNode* value;
            ASTNode* template_data;
        } return_stmt;
        
        // Try-Catch
        struct {
            NodeList* try_block;
            char* catch_var;
            NodeList* catch_block;
            NodeList* finally_block;
        } try_catch;
        
        // Binary Operation
        struct {
            char* op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        
        // Unary Operation
        struct {
            char* op;
            ASTNode* operand;
            bool prefix;
        } unary_op;
        
        // Ternary Operation
        struct {
            ASTNode* condition;
            ASTNode* then_expr;
            ASTNode* else_expr;
        } ternary_op;
        
        // Identifier
        struct {
            char* name;
        } identifier;
        
        // Literals
        struct { long value; } int_literal;
        struct { double value; } float_literal;
        struct { char* value; } string_literal;
        struct { char* value; NodeList* expressions; } template_string;
        struct { bool value; } bool_literal;
        
        // Array
        struct {
            NodeList* elements;
        } array;
        
        // Object
        struct {
            NodeList* entries;
        } object;
        
        // Object Entry
        struct {
            char* key;
            ASTNode* value;
            bool computed;
        } object_entry;
        
        // Member Access
        struct {
            ASTNode* object;
            char* member;
        } member_access;
        
        // Index Access
        struct {
            ASTNode* object;
            ASTNode* index;
        } index_access;
        
        // Route Parameter reference
        struct {
            char* param_name;
        } route_param;
        
        // SEO Block
        struct {
            NodeList* entries;
        } seo_block;
        
        // Middleware
        struct {
            char* name;
            NodeList* body;
        } middleware;
        
    } data;
    
    int line;
    int column;
};

// ============================================================================
// Function Declarations
// ============================================================================

// Lexer
Token* lexer_tokenize(const char* source);
void lexer_free_tokens(Token* tokens);

// Parser
ASTNode* parser_parse(Token* tokens);
void ast_free(ASTNode* node);
void ast_print(ASTNode* node, int indent);

// Code Generator
char* codegen_generate(ASTNode* program);
void codegen_free(char* code);

// Utility
void error(const char* fmt, ...);
char* string_duplicate(const char* str);

#endif // WEBFAST_H
