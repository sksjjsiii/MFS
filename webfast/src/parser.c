/*
 * WebFast Parser - Recursive Descent Parser (Fixed Version)
 * Converts tokens into Abstract Syntax Tree (AST)
 */

#include "../include/webfast.h"
#include <stdarg.h>

// Parser state
typedef struct {
    Token* tokens;
    size_t pos;
} Parser;

// Forward declarations
static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_primary(Parser* p);
static NodeList* parse_statements(Parser* p);

// Node creation helpers
static ASTNode* node_create(NodeType type, int line, int col) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    if (!node) {
        error("Memory allocation failed");
        return NULL;
    }
    node->type = type;
    node->line = line;
    node->column = col;
    return node;
}

static NodeList* nodelist_create(ASTNode* node) {
    NodeList* list = malloc(sizeof(NodeList));
    if (!list) {
        error("Memory allocation failed");
        return NULL;
    }
    list->node = node;
    list->next = NULL;
    return list;
}

static void nodelist_append(NodeList** list, ASTNode* node) {
    NodeList* new_item = nodelist_create(node);
    if (!*list) {
        *list = new_item;
    } else {
        NodeList* current = *list;
        while (current->next) {
            current = current->next;
        }
        current->next = new_item;
    }
}

// Parser helpers
static Token* current(Parser* p) {
    return &p->tokens[p->pos];
}

static Token* advance(Parser* p) {
    Token* tok = current(p);
    p->pos++;
    return tok;
}

static bool check(Parser* p, TokenType type) {
    return current(p)->type == type;
}

static bool match(Parser* p, TokenType type) {
    if (check(p, type)) {
        advance(p);
        return true;
    }
    return false;
}

static Token* expect(Parser* p, TokenType type, const char* msg) {
    if (check(p, type)) {
        return advance(p);
    }
    error("%s at line %d, column %d (got %s)", msg, current(p)->line, current(p)->column, current(p)->value);
    return NULL;
}

// Parse config block
static ASTNode* parse_config(Parser* p) {
    Token* tok = advance(p); // consume 'config'
    ASTNode* node = node_create(NODE_CONFIG, tok->line, tok->column);
    
    expect(p, TOK_LBRACE, "Expected '{' after 'config'");
    
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Token* key_tok = expect(p, TOK_IDENTIFIER, "Expected identifier in config");
        expect(p, TOK_COLON, "Expected ':' after config key");
        
        ASTNode* value = parse_expression(p);
        
        ASTNode* entry = node_create(NODE_CONFIG_ENTRY, key_tok->line, key_tok->column);
        entry->data.config_entry.key = string_duplicate(key_tok->value);
        entry->data.config_entry.value = value;
        
        nodelist_append(&node->data.config.entries, entry);
    }
    
    expect(p, TOK_RBRACE, "Expected '}' after config block");
    
    return node;
}

// Parse route path with parameters
static void parse_route_path(const char* path, NodeList** params) {
    char* path_copy = string_duplicate(path);
    char* token = strtok(path_copy, "/");
    
    while (token) {
        if (token[0] == '{' && token[strlen(token)-1] == '}') {
            size_t len = strlen(token) - 2;
            char* param_name = malloc(len + 1);
            strncpy(param_name, token + 1, len);
            param_name[len] = '\0';
            
            ASTNode* param_node = node_create(NODE_ROUTE_PARAM, 0, 0);
            param_node->data.route_param.param_name = param_name;
            nodelist_append(params, param_node);
        }
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
}

// Parse route
static ASTNode* parse_route(Parser* p) {
    Token* tok = advance(p); // consume 'route'
    
    Token* method_tok = advance(p);
    if (method_tok->type != TOK_GET && method_tok->type != TOK_POST &&
        method_tok->type != TOK_PUT && method_tok->type != TOK_DELETE &&
        method_tok->type != TOK_PATCH) {
        error("Expected HTTP method after 'route' at line %d", method_tok->line);
    }
    
    Token* path_tok = expect(p, TOK_STRING, "Expected path string after HTTP method");
    
    ASTNode* node = node_create(NODE_ROUTE, tok->line, tok->column);
    node->data.route.method = string_duplicate(method_tok->value);
    node->data.route.path = string_duplicate(path_tok->value);
    node->data.route.params = NULL;
    
    // Parse path parameters
    parse_route_path(path_tok->value, &node->data.route.params);
    
    expect(p, TOK_LBRACE, "Expected '{' after route path");
    
    node->data.route.statements = parse_statements(p);
    
    expect(p, TOK_RBRACE, "Expected '}' after route block");
    
    return node;
}

// Parse SEO block
static ASTNode* parse_seo_block(Parser* p) {
    Token* tok = advance(p); // consume 'seo'
    ASTNode* node = node_create(NODE_SEO_BLOCK, tok->line, tok->column);
    
    expect(p, TOK_LBRACE, "Expected '{' after 'seo'");
    
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Token* key_tok = expect(p, TOK_IDENTIFIER, "Expected identifier in SEO block");
        expect(p, TOK_COLON, "Expected ':' after SEO key");
        
        ASTNode* value = parse_expression(p);
        
        ASTNode* entry = node_create(NODE_OBJECT_ENTRY, key_tok->line, key_tok->column);
        entry->data.object_entry.key = string_duplicate(key_tok->value);
        entry->data.object_entry.value = value;
        
        nodelist_append(&node->data.seo_block.entries, entry);
    }
    
    expect(p, TOK_RBRACE, "Expected '}' after SEO block");
    
    return node;
}

// Parse variable declaration/assignment
static ASTNode* parse_var_decl_or_assign(Parser* p) {
    Token* id_tok = advance(p); // consume identifier
    
    if (match(p, TOK_ASSIGN)) {
        // Assignment
        ASTNode* value = parse_expression(p);
        
        ASTNode* node = node_create(NODE_VAR_ASSIGN, id_tok->line, id_tok->column);
        ASTNode* target = node_create(NODE_IDENTIFIER, id_tok->line, id_tok->column);
        target->data.identifier.name = string_duplicate(id_tok->value);
        node->data.var_assign.target = target;
        node->data.var_assign.value = value;
        
        return node;
    }
    
    error("Expected '=' after identifier at line %d", id_tok->line);
    return NULL;
}

// Parse if statement
static ASTNode* parse_if_stmt(Parser* p) {
    Token* tok = advance(p); // consume 'if'
    
    ASTNode* condition = parse_expression(p);
    
    expect(p, TOK_LBRACE, "Expected '{' after if condition");
    NodeList* then_branch = parse_statements(p);
    expect(p, TOK_RBRACE, "Expected '}' after if body");
    
    NodeList* else_branch = NULL;
    if (match(p, TOK_ELSE)) {
        expect(p, TOK_LBRACE, "Expected '{' after else");
        else_branch = parse_statements(p);
        expect(p, TOK_RBRACE, "Expected '}' after else body");
    }
    
    ASTNode* node = node_create(NODE_IF_STMT, tok->line, tok->column);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;
    
    return node;
}

// Parse for statement
static ASTNode* parse_for_stmt(Parser* p) {
    Token* tok = advance(p); // consume 'for'
    
    Token* var_tok = expect(p, TOK_IDENTIFIER, "Expected variable name after 'for'");
    expect(p, TOK_IN, "Expected 'in' after variable name");
    
    ASTNode* iterable = parse_expression(p);
    
    expect(p, TOK_LBRACE, "Expected '{' after for expression");
    NodeList* body = parse_statements(p);
    expect(p, TOK_RBRACE, "Expected '}' after for body");
    
    ASTNode* node = node_create(NODE_FOR_STMT, tok->line, tok->column);
    node->data.for_stmt.var_name = string_duplicate(var_tok->value);
    node->data.for_stmt.iterable = iterable;
    node->data.for_stmt.body = body;
    
    return node;
}

// Parse return statement
static ASTNode* parse_return_stmt(Parser* p) {
    Token* tok = advance(p); // consume 'return'
    
    Token* type_tok = expect(p, TOK_IDENTIFIER, "Expected return type (json, html, sitemap, text)");
    
    ASTNode* value = parse_expression(p);
    
    ASTNode* template_data = NULL;
    if (strcmp(type_tok->value, "html") == 0 && match(p, TOK_LBRACE)) {
        // Parse template data
        template_data = node_create(NODE_OBJECT, current(p)->line, current(p)->column);
        
        while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
            Token* key_tok = expect(p, TOK_IDENTIFIER, "Expected identifier in template data");
            expect(p, TOK_COLON, "Expected ':' after key");
            
            ASTNode* val = parse_expression(p);
            
            ASTNode* entry = node_create(NODE_OBJECT_ENTRY, key_tok->line, key_tok->column);
            entry->data.object_entry.key = string_duplicate(key_tok->value);
            entry->data.object_entry.value = val;
            
            nodelist_append(&template_data->data.object.entries, entry);
        }
        
        expect(p, TOK_RBRACE, "Expected '}' after template data");
    }
    
    ASTNode* node = node_create(NODE_RETURN_STMT, tok->line, tok->column);
    node->data.return_stmt.return_type = string_duplicate(type_tok->value);
    node->data.return_stmt.value = value;
    node->data.return_stmt.template_data = template_data;
    
    return node;
}

// Parse statements (FIXED: only handles valid statements, not config/route)
static NodeList* parse_statements(Parser* p) {
    NodeList* stmts = NULL;
    
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        ASTNode* stmt = NULL;
        
        if (check(p, TOK_SEO)) {
            stmt = parse_seo_block(p);
        } else if (check(p, TOK_IF)) {
            stmt = parse_if_stmt(p);
        } else if (check(p, TOK_FOR)) {
            stmt = parse_for_stmt(p);
        } else if (check(p, TOK_RETURN)) {
            stmt = parse_return_stmt(p);
        } else if (check(p, TOK_IDENTIFIER)) {
            stmt = parse_var_decl_or_assign(p);
        } else {
            error("Unexpected token '%s' at line %d", current(p)->value, current(p)->line);
            advance(p);
            continue;
        }
        
        if (stmt) {
            nodelist_append(&stmts, stmt);
        }
    }
    
    return stmts;
}

// Parse primary expression
static ASTNode* parse_primary(Parser* p) {
    Token* tok = current(p);
    
    switch (tok->type) {
        case TOK_INTEGER: {
            advance(p);
            ASTNode* node = node_create(NODE_INT_LITERAL, tok->line, tok->column);
            node->data.int_literal.value = atol(tok->value);
            return node;
        }
        
        case TOK_FLOAT: {
            advance(p);
            ASTNode* node = node_create(NODE_FLOAT_LITERAL, tok->line, tok->column);
            node->data.float_literal.value = atof(tok->value);
            return node;
        }
        
        case TOK_STRING: {
            advance(p);
            ASTNode* node = node_create(NODE_STRING_LITERAL, tok->line, tok->column);
            node->data.string_literal.value = string_duplicate(tok->value);
            return node;
        }
        
        case TOK_TRUE: {
            advance(p);
            ASTNode* node = node_create(NODE_BOOL_LITERAL, tok->line, tok->column);
            node->data.bool_literal.value = true;
            return node;
        }
        
        case TOK_FALSE: {
            advance(p);
            ASTNode* node = node_create(NODE_BOOL_LITERAL, tok->line, tok->column);
            node->data.bool_literal.value = false;
            return node;
        }
        
        case TOK_NULL: {
            advance(p);
            return node_create(NODE_NULL_LITERAL, tok->line, tok->column);
        }
        
        case TOK_NOW: {
            advance(p);
            return node_create(NODE_NOW_EXPR, tok->line, tok->column);
        }
        
        case TOK_QUERY: {
            advance(p);
            return node_create(NODE_QUERY_EXPR, tok->line, tok->column);
        }
        
        case TOK_CONFIG: {
            advance(p);
            // Treat 'config' as identifier for member access (e.g., config.site_name)
            if (check(p, TOK_DOT)) {
                advance(p);
                Token* member_tok = expect(p, TOK_IDENTIFIER, "Expected member name after '.'");
                
                ASTNode* obj = node_create(NODE_IDENTIFIER, tok->line, tok->column);
                obj->data.identifier.name = string_duplicate(tok->value);
                
                ASTNode* node = node_create(NODE_MEMBER_ACCESS, tok->line, tok->column);
                node->data.member_access.object = obj;
                node->data.member_access.member = string_duplicate(member_tok->value);
                
                return node;
            }
            
            ASTNode* node = node_create(NODE_IDENTIFIER, tok->line, tok->column);
            node->data.identifier.name = string_duplicate(tok->value);
            return node;
        }
        
        case TOK_IDENTIFIER: {
            advance(p);
            
            // Check for member access
            if (check(p, TOK_DOT)) {
                advance(p); // consume '.'
                Token* member_tok = expect(p, TOK_IDENTIFIER, "Expected member name after '.'");
                
                ASTNode* obj = node_create(NODE_IDENTIFIER, tok->line, tok->column);
                obj->data.identifier.name = string_duplicate(tok->value);
                
                ASTNode* node = node_create(NODE_MEMBER_ACCESS, tok->line, tok->column);
                node->data.member_access.object = obj;
                node->data.member_access.member = string_duplicate(member_tok->value);
                
                return node;
            }
            
            ASTNode* node = node_create(NODE_IDENTIFIER, tok->line, tok->column);
            node->data.identifier.name = string_duplicate(tok->value);
            return node;
        }
        
        case TOK_LBRACKET: {
            advance(p); // consume '['
            ASTNode* node = node_create(NODE_ARRAY, tok->line, tok->column);
            
            while (!check(p, TOK_RBRACKET) && !check(p, TOK_EOF)) {
                ASTNode* elem = parse_expression(p);
                nodelist_append(&node->data.array.elements, elem);
                
                if (!match(p, TOK_COMMA)) {
                    break;
                }
            }
            
            expect(p, TOK_RBRACKET, "Expected ']' after array");
            return node;
        }
        
        case TOK_LBRACE: {
            advance(p); // consume '{'
            ASTNode* node = node_create(NODE_OBJECT, tok->line, tok->column);
            
            while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
                /* Allow keywords as object keys too */
                Token* key_tok = current(p);
                if (key_tok->type == TOK_IDENTIFIER ||
                    key_tok->type == TOK_NOW ||
                    key_tok->type == TOK_QUERY ||
                    key_tok->type == TOK_BODY ||
                    key_tok->type == TOK_HEADERS ||
                    key_tok->type == TOK_PARAMS ||
                    key_tok->type == TOK_CONFIG ||
                    key_tok->type == TOK_ROUTE ||
                    key_tok->type == TOK_GET ||
                    key_tok->type == TOK_POST ||
                    key_tok->type == TOK_PUT ||
                    key_tok->type == TOK_DELETE ||
                    key_tok->type == TOK_PATCH ||
                    key_tok->type == TOK_HEAD ||
                    key_tok->type == TOK_OPTIONS ||
                    key_tok->type == TOK_SEO ||
                    key_tok->type == TOK_RETURN ||
                    key_tok->type == TOK_IF ||
                    key_tok->type == TOK_ELSE ||
                    key_tok->type == TOK_FOR ||
                    key_tok->type == TOK_IN ||
                    key_tok->type == TOK_WHILE ||
                    key_tok->type == TOK_DO ||
                    key_tok->type == TOK_BREAK ||
                    key_tok->type == TOK_CONTINUE ||
                    key_tok->type == TOK_NULL ||
                    key_tok->type == TOK_TRUE ||
                    key_tok->type == TOK_FALSE ||
                    key_tok->type == TOK_FUNCTION ||
                    key_tok->type == TOK_LET ||
                    key_tok->type == TOK_CONST ||
                    key_tok->type == TOK_VAR ||
                    key_tok->type == TOK_TYPE ||
                    key_tok->type == TOK_INTERFACE ||
                    key_tok->type == TOK_STRUCT ||
                    key_tok->type == TOK_CLASS ||
                    key_tok->type == TOK_PUBLIC ||
                    key_tok->type == TOK_PRIVATE ||
                    key_tok->type == TOK_PROTECTED ||
                    key_tok->type == TOK_STATIC ||
                    key_tok->type == TOK_EXTENDS ||
                    key_tok->type == TOK_IMPLEMENTS ||
                    key_tok->type == TOK_NEW ||
                    key_tok->type == TOK_THIS ||
                    key_tok->type == TOK_SELF ||
                    key_tok->type == TOK_SUPER ||
                    key_tok->type == TOK_ASYNC ||
                    key_tok->type == TOK_AWAIT ||
                    key_tok->type == TOK_TRY ||
                    key_tok->type == TOK_CATCH ||
                    key_tok->type == TOK_FINALLY ||
                    key_tok->type == TOK_THROW ||
                    key_tok->type == TOK_ERROR ||
                    key_tok->type == TOK_IMPORT ||
                    key_tok->type == TOK_EXPORT ||
                    key_tok->type == TOK_USE ||
                    key_tok->type == TOK_MIDDLEWARE ||
                    key_tok->type == TOK_BEFORE ||
                    key_tok->type == TOK_AFTER) {
                    advance(p);
                } else {
                    error("Expected key in object at line %d, column %d (got %s)", 
                          key_tok->line, key_tok->column, key_tok->value);
                    return NULL;
                }
                expect(p, TOK_COLON, "Expected ':' after object key");
                
                ASTNode* value = parse_expression(p);
                
                ASTNode* entry = node_create(NODE_OBJECT_ENTRY, key_tok->line, key_tok->column);
                entry->data.object_entry.key = string_duplicate(key_tok->value);
                entry->data.object_entry.value = value;
                
                nodelist_append(&node->data.object.entries, entry);
                
                if (!match(p, TOK_COMMA)) {
                    break;
                }
            }
            
            expect(p, TOK_RBRACE, "Expected '}' after object");
            return node;
        }
        
        case TOK_LPAREN: {
            advance(p); // consume '('
            ASTNode* expr = parse_expression(p);
            expect(p, TOK_RPAREN, "Expected ')' after expression");
            return expr;
        }
        
        default:
            error("Unexpected token '%s' at line %d", tok->value, tok->line);
            return NULL;
    }
}

// Parse unary expression
static ASTNode* parse_unary(Parser* p) {
    if (match(p, TOK_NOT)) {
        Token* op = &p->tokens[p->pos - 1];
        ASTNode* operand = parse_unary(p);
        
        ASTNode* node = node_create(NODE_UNARY_OP, op->line, op->column);
        node->data.unary_op.op = string_duplicate(op->value);
        node->data.unary_op.operand = operand;
        
        return node;
    }
    
    if (match(p, TOK_MINUS)) {
        Token* op = &p->tokens[p->pos - 1];
        ASTNode* operand = parse_unary(p);
        
        ASTNode* node = node_create(NODE_UNARY_OP, op->line, op->column);
        node->data.unary_op.op = string_duplicate(op->value);
        node->data.unary_op.operand = operand;
        
        return node;
    }
    
    return parse_primary(p);
}

// Parse binary expression (with precedence)
static ASTNode* parse_expression(Parser* p) {
    ASTNode* left = parse_unary(p);
    
    while (check(p, TOK_EQUAL) || check(p, TOK_NOT_EQUAL) ||
           check(p, TOK_LESS) || check(p, TOK_GREATER) ||
           check(p, TOK_LESS_EQ) || check(p, TOK_GREATER_EQ) ||
           check(p, TOK_AND) || check(p, TOK_OR) ||
           check(p, TOK_PLUS) || check(p, TOK_MINUS) ||
           check(p, TOK_MULTIPLY) || check(p, TOK_DIVIDE) ||
           check(p, TOK_MODULO)) {
        
        Token* op_tok = advance(p);
        ASTNode* right = parse_unary(p);
        
        ASTNode* node = node_create(NODE_BINARY_OP, op_tok->line, op_tok->column);
        node->data.binary_op.op = string_duplicate(op_tok->value);
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        
        left = node;
    }
    
    return left;
}

// Main parse function
ASTNode* parser_parse(Token* tokens) {
    if (!tokens) return NULL;
    
    Parser p = { tokens, 0 };
    
    ASTNode* program = node_create(NODE_PROGRAM, 1, 1);
    program->data.program.configs = NULL;
    program->data.program.routes = NULL;
    
    while (!check(&p, TOK_EOF)) {
        if (check(&p, TOK_CONFIG)) {
            ASTNode* config = parse_config(&p);
            nodelist_append(&program->data.program.configs, config);
        } else if (check(&p, TOK_ROUTE)) {
            ASTNode* route = parse_route(&p);
            nodelist_append(&program->data.program.routes, route);
        } else {
            error("Unexpected token '%s' at top level", current(&p)->value);
            advance(&p);
        }
    }
    
    return program;
}

// AST memory management (same as before)
void ast_free(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_PROGRAM: {
            NodeList* curr = node->data.program.configs;
            while (curr) {
                NodeList* next = curr->next;
                ast_free(curr->node);
                free(curr);
                curr = next;
            }
            curr = node->data.program.routes;
            while (curr) {
                NodeList* next = curr->next;
                ast_free(curr->node);
                free(curr);
                curr = next;
            }
            break;
        }
        
        case NODE_CONFIG: {
            NodeList* curr = node->data.config.entries;
            while (curr) {
                NodeList* next = curr->next;
                ast_free(curr->node);
                free(curr);
                curr = next;
            }
            break;
        }
        
        case NODE_CONFIG_ENTRY:
            free(node->data.config_entry.key);
            ast_free(node->data.config_entry.value);
            break;
        
        case NODE_ROUTE:
            free(node->data.route.method);
            free(node->data.route.path);
            
            NodeList* param = node->data.route.params;
            while (param) {
                NodeList* next = param->next;
                free(param->node->data.route_param.param_name);
                free(param->node);
                free(param);
                param = next;
            }
            
            NodeList* stmt = node->data.route.statements;
            while (stmt) {
                NodeList* next = stmt->next;
                ast_free(stmt->node);
                free(stmt);
                stmt = next;
            }
            break;
        
        case NODE_SEO_BLOCK: {
            NodeList* curr = node->data.seo_block.entries;
            while (curr) {
                NodeList* next = curr->next;
                ast_free(curr->node);
                free(curr);
                curr = next;
            }
            break;
        }
        
        case NODE_VAR_ASSIGN:
            ast_free(node->data.var_assign.target);
            ast_free(node->data.var_assign.value);
            break;
        
        case NODE_IF_STMT:
            ast_free(node->data.if_stmt.condition);
            {
                NodeList* curr = node->data.if_stmt.then_branch;
                while (curr) {
                    NodeList* next = curr->next;
                    ast_free(curr->node);
                    free(curr);
                    curr = next;
                }
            }
            if (node->data.if_stmt.else_branch) {
                NodeList* curr = node->data.if_stmt.else_branch;
                while (curr) {
                    NodeList* next = curr->next;
                    ast_free(curr->node);
                    free(curr);
                    curr = next;
                }
            }
            break;
        
        case NODE_FOR_STMT:
            free(node->data.for_stmt.var_name);
            ast_free(node->data.for_stmt.iterable);
            {
                NodeList* curr = node->data.for_stmt.body;
                while (curr) {
                    NodeList* next = curr->next;
                    ast_free(curr->node);
                    free(curr);
                    curr = next;
                }
            }
            break;
        
        case NODE_RETURN_STMT:
            free(node->data.return_stmt.return_type);
            ast_free(node->data.return_stmt.value);
            ast_free(node->data.return_stmt.template_data);
            break;
        
        case NODE_BINARY_OP:
            free(node->data.binary_op.op);
            ast_free(node->data.binary_op.left);
            ast_free(node->data.binary_op.right);
            break;
        
        case NODE_UNARY_OP:
            free(node->data.unary_op.op);
            ast_free(node->data.unary_op.operand);
            break;
        
        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        
        case NODE_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;
        
        case NODE_ARRAY: {
            NodeList* curr = node->data.array.elements;
            while (curr) {
                NodeList* next = curr->next;
                ast_free(curr->node);
                free(curr);
                curr = next;
            }
            break;
        }
        
        case NODE_OBJECT: {
            NodeList* curr = node->data.object.entries;
            while (curr) {
                NodeList* next = curr->next;
                ast_free(curr->node);
                free(curr);
                curr = next;
            }
            break;
        }
        
        case NODE_OBJECT_ENTRY:
            free(node->data.object_entry.key);
            ast_free(node->data.object_entry.value);
            break;
        
        case NODE_MEMBER_ACCESS:
            ast_free(node->data.member_access.object);
            free(node->data.member_access.member);
            break;
        
        case NODE_ROUTE_PARAM:
            free(node->data.route_param.param_name);
            break;
        
        default:
            break;
    }
    
    free(node);
}

// AST printing
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(ASTNode* node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    
    switch (node->type) {
        case NODE_PROGRAM:
            printf("PROGRAM\n");
            {
                NodeList* curr = node->data.program.configs;
                while (curr) {
                    ast_print(curr->node, indent + 1);
                    curr = curr->next;
                }
                curr = node->data.program.routes;
                while (curr) {
                    ast_print(curr->node, indent + 1);
                    curr = curr->next;
                }
            }
            break;
        
        case NODE_CONFIG:
            printf("CONFIG\n");
            {
                NodeList* curr = node->data.config.entries;
                while (curr) {
                    ast_print(curr->node, indent + 1);
                    curr = curr->next;
                }
            }
            break;
        
        case NODE_CONFIG_ENTRY:
            printf("CONFIG_ENTRY: %s\n", node->data.config_entry.key);
            ast_print(node->data.config_entry.value, indent + 1);
            break;
        
        case NODE_ROUTE:
            printf("ROUTE: %s %s\n", node->data.route.method, node->data.route.path);
            {
                NodeList* curr = node->data.route.statements;
                while (curr) {
                    ast_print(curr->node, indent + 1);
                    curr = curr->next;
                }
            }
            break;
        
        case NODE_RETURN_STMT:
            printf("RETURN: %s\n", node->data.return_stmt.return_type);
            ast_print(node->data.return_stmt.value, indent + 1);
            break;
        
        case NODE_STRING_LITERAL:
            printf("STRING: \"%s\"\n", node->data.string_literal.value);
            break;
        
        case NODE_INT_LITERAL:
            printf("INT: %ld\n", node->data.int_literal.value);
            break;
        
        case NODE_FLOAT_LITERAL:
            printf("FLOAT: %f\n", node->data.float_literal.value);
            break;
        
        case NODE_BOOL_LITERAL:
            printf("BOOL: %s\n", node->data.bool_literal.value ? "true" : "false");
            break;
        
        case NODE_IDENTIFIER:
            printf("IDENT: %s\n", node->data.identifier.name);
            break;
        
        default:
            printf("NODE: %d\n", node->type);
            break;
    }
}
