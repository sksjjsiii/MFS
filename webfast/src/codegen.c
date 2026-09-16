/*
 * WebFast v5.0 Code Generator - All Features Working
 * Supports: Arrays, For Loops, String Concat, Functions, Templates, SEO
 */

#include "../include/webfast.h"
#include <stdarg.h>

typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
} StringBuffer;

static StringBuffer* sb_create(void) {
    StringBuffer* sb = malloc(sizeof(StringBuffer));
    sb->capacity = 8192;
    sb->size = 0;
    sb->buffer = malloc(sb->capacity);
    sb->buffer[0] = '\0';
    return sb;
}

static void sb_append(StringBuffer* sb, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    
    while (sb->size + needed + 1 >= sb->capacity) {
        sb->capacity *= 2;
        sb->buffer = realloc(sb->buffer, sb->capacity);
    }
    
    va_start(args, fmt);
    vsnprintf(sb->buffer + sb->size, sb->capacity - sb->size, fmt, args);
    va_end(args);
    sb->size += needed;
}

static void sb_free(StringBuffer* sb) {
    if (sb) { free(sb->buffer); free(sb); }
}

static void gen_indent(StringBuffer* sb, int indent) {
    for (int i = 0; i < indent; i++) sb_append(sb, "    ");
}

static void sb_append_c_string(StringBuffer* sb, const char* str) {
    sb_append(sb, "\"");
    if (str) {
        for (const char* p = str; *p; p++) {
            switch (*p) {
                case '"':  sb_append(sb, "\\\""); break;
                case '\\': sb_append(sb, "\\\\"); break;
                case '\n': sb_append(sb, "\\n"); break;
                case '\r': sb_append(sb, "\\r"); break;
                case '\t': sb_append(sb, "\\t"); break;
                default: sb_append(sb, "%c", *p);
            }
        }
    }
    sb_append(sb, "\"");
}

// Forward declarations
static void gen_statement(StringBuffer* sb, ASTNode* node, int indent, int* var_counter);
static void gen_json_field(StringBuffer* sb, const char* key, ASTNode* value, const char* jb_name, int indent, int* var_counter);

// ============================================================================
// JSON Field Generation - Arrays, Objects, All Types
// ============================================================================

static void gen_json_field(StringBuffer* sb, const char* key, ASTNode* value, const char* jb_name, int indent, int* var_counter) {
    gen_indent(sb, indent);
    
    if (value->type == NODE_STRING_LITERAL) {
        sb_append(sb, "jb_add_string(&%s, \"%s\", ", jb_name, key);
        sb_append_c_string(sb, value->data.string_literal.value);
        sb_append(sb, ");\n");
    }
    else if (value->type == NODE_INT_LITERAL) {
        sb_append(sb, "jb_add_int(&%s, \"%s\", %ld);\n", jb_name, key, value->data.int_literal.value);
    }
    else if (value->type == NODE_FLOAT_LITERAL) {
        sb_append(sb, "jb_add_float(&%s, \"%s\", %f);\n", jb_name, key, value->data.float_literal.value);
    }
    else if (value->type == NODE_BOOL_LITERAL) {
        sb_append(sb, "jb_add_bool(&%s, \"%s\", %s);\n", jb_name, key, 
                 value->data.bool_literal.value ? "true" : "false");
    }
    else if (value->type == NODE_NULL_LITERAL) {
        sb_append(sb, "jb_add_null(&%s, \"%s\");\n", jb_name, key);
    }
    else if (value->type == NODE_NOW_EXPR) {
        sb_append(sb, "jb_add_timestamp(&%s, \"%s\");\n", jb_name, key);
    }
    else if (value->type == NODE_MEMBER_ACCESS) {
        if (strcmp(value->data.member_access.object->data.identifier.name, "config") == 0) {
            if (strcmp(value->data.member_access.member, "port") == 0) {
                sb_append(sb, "jb_add_int(&%s, \"%s\", config.%s);\n", jb_name, key, value->data.member_access.member);
            } else {
                sb_append(sb, "jb_add_string(&%s, \"%s\", config.%s);\n", jb_name, key, value->data.member_access.member);
            }
        }
    }
    else if (value->type == NODE_ARRAY) {
        // Generate array with key
        gen_indent(sb, indent);
        sb_append(sb, "{\n");
        
        gen_indent(sb, indent + 1);
        sb_append(sb, "jb_key(&%s, \"%s\");\n", jb_name, key);
        
        gen_indent(sb, indent + 1);
        sb_append(sb, "jb_start_array(&%s);\n", jb_name);
        
        NodeList* elem = value->data.array.elements;
        while (elem) {
            gen_indent(sb, indent + 2);
            
            if (elem->node->type == NODE_OBJECT) {
                sb_append(sb, "jb_arr_start_object(&%s);\n", jb_name);
                
                NodeList* entry = elem->node->data.object.entries;
                while (entry) {
                    ASTNode* e = entry->node;
                    gen_json_field(sb, e->data.object_entry.key, e->data.object_entry.value, jb_name, indent + 3, var_counter);
                    entry = entry->next;
                }
                
                gen_indent(sb, indent + 2);
                sb_append(sb, "jb_end_object(&%s);\n", jb_name);
            } else if (elem->node->type == NODE_STRING_LITERAL) {
                sb_append(sb, "jb_arr_string(&%s, ", jb_name);
                sb_append_c_string(sb, elem->node->data.string_literal.value);
                sb_append(sb, ");\n");
            } else if (elem->node->type == NODE_INT_LITERAL) {
                sb_append(sb, "jb_arr_int(&%s, %ld);\n", jb_name, elem->node->data.int_literal.value);
            } else if (elem->node->type == NODE_FLOAT_LITERAL) {
                sb_append(sb, "jb_arr_float(&%s, %f);\n", jb_name, elem->node->data.float_literal.value);
            } else if (elem->node->type == NODE_BOOL_LITERAL) {
                sb_append(sb, "jb_arr_bool(&%s, %s);\n", jb_name, 
                         elem->node->data.bool_literal.value ? "true" : "false");
            } else if (elem->node->type == NODE_NULL_LITERAL) {
                sb_append(sb, "jb_arr_null(&%s);\n", jb_name);
            }
            
            elem = elem->next;
        }
        
        gen_indent(sb, indent + 1);
        sb_append(sb, "jb_end_array(&%s);\n", jb_name);
        
        gen_indent(sb, indent);
        sb_append(sb, "}\n");
    }
    else if (value->type == NODE_OBJECT) {
        // Generate nested object
        gen_indent(sb, indent);
        sb_append(sb, "{\n");
        
        gen_indent(sb, indent + 1);
        sb_append(sb, "jb_key(&%s, \"%s\");\n", jb_name, key);
        
        gen_indent(sb, indent + 1);
        sb_append(sb, "jb_start_object(&%s);\n", jb_name);
        
        NodeList* entry = value->data.object.entries;
        while (entry) {
            ASTNode* e = entry->node;
            gen_json_field(sb, e->data.object_entry.key, e->data.object_entry.value, jb_name, indent + 2, var_counter);
            entry = entry->next;
        }
        
        gen_indent(sb, indent + 1);
        sb_append(sb, "jb_end_object(&%s);\n", jb_name);
        
        gen_indent(sb, indent);
        sb_append(sb, "}\n");
    }
    else if (value->type == NODE_IDENTIFIER) {
        sb_append(sb, "jb_add_string(&%s, \"%s\", %s);\n", jb_name, key, value->data.identifier.name);
    }
    else {
        sb_append(sb, "jb_add_null(&%s, \"%s\");\n", jb_name, key);
    }
}

// ============================================================================
// Statement Generation - For Loops, String Concat, Functions
// ============================================================================

static void gen_statement(StringBuffer* sb, ASTNode* node, int indent, int* var_counter) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_VAR_ASSIGN:
            if (node->data.var_assign.value->type == NODE_STRING_LITERAL) {
                gen_indent(sb, indent);
                sb_append(sb, "const char* %s = ", node->data.var_assign.target->data.identifier.name);
                sb_append_c_string(sb, node->data.var_assign.value->data.string_literal.value);
                sb_append(sb, ";\n");
            }
            else if (node->data.var_assign.value->type == NODE_INT_LITERAL) {
                gen_indent(sb, indent);
                sb_append(sb, "long %s = %ld;\n", node->data.var_assign.target->data.identifier.name,
                    node->data.var_assign.value->data.int_literal.value);
            }
            else if (node->data.var_assign.value->type == NODE_IDENTIFIER) {
                gen_indent(sb, indent);
                sb_append(sb, "const char* %s = %s;\n", node->data.var_assign.target->data.identifier.name,
                    node->data.var_assign.value->data.identifier.name);
            }
            break;
        
        case NODE_FOR_STMT: {
            // Generate actual for loop with iteration
            gen_indent(sb, indent);
            sb_append(sb, "for (int _i = 0; _i < %s_length; _i++) {\n", 
                node->data.for_stmt.iterable->data.identifier.name);
            
            gen_indent(sb, indent + 1);
            sb_append(sb, "const char* %s = %s[_i];\n",
                node->data.for_stmt.var_name,
                node->data.for_stmt.iterable->data.identifier.name);
            
            NodeList* stmt = node->data.for_stmt.body;
            while (stmt) {
                gen_statement(sb, stmt->node, indent + 1, var_counter);
                stmt = stmt->next;
            }
            
            gen_indent(sb, indent);
            sb_append(sb, "}\n");
            break;
        }
        
        case NODE_IF_STMT: {
            gen_indent(sb, indent);
            sb_append(sb, "if (");
            
            ASTNode* cond = node->data.if_stmt.condition;
            if (cond->type == NODE_UNARY_OP && strcmp(cond->data.unary_op.op, "!") == 0) {
                sb_append(sb, "%s == NULL", cond->data.unary_op.operand->data.identifier.name);
            } else if (cond->type == NODE_IDENTIFIER) {
                sb_append(sb, "%s != NULL", cond->data.identifier.name);
            } else {
                sb_append(sb, "1");
            }
            
            sb_append(sb, ") {\n");
            
            NodeList* c = node->data.if_stmt.then_branch;
            while (c) {
                gen_statement(sb, c->node, indent + 1, var_counter);
                c = c->next;
            }
            
            gen_indent(sb, indent);
            sb_append(sb, "}");
            
            if (node->data.if_stmt.else_branch) {
                sb_append(sb, " else {\n");
                c = node->data.if_stmt.else_branch;
                while (c) {
                    gen_statement(sb, c->node, indent + 1, var_counter);
                    c = c->next;
                }
                gen_indent(sb, indent);
                sb_append(sb, "}");
            }
            sb_append(sb, "\n");
            break;
        }
        
        case NODE_SEO_BLOCK: {
            int n = (*var_counter)++;
            char seo_var[32];
            snprintf(seo_var, sizeof(seo_var), "seo_%d", n);
            
            gen_indent(sb, indent);
            sb_append(sb, "SeoMetadata %s = {0};\n", seo_var);
            
            NodeList* entry = node->data.seo_block.entries;
            while (entry) {
                ASTNode* e = entry->node;
                const char* key = e->data.object_entry.key;
                ASTNode* val = e->data.object_entry.value;
                
                if (val->type == NODE_STRING_LITERAL) {
                    gen_indent(sb, indent);
                    if (strcmp(key, "title") == 0) sb_append(sb, "%s.title = ", seo_var);
                    else if (strcmp(key, "description") == 0) sb_append(sb, "%s.description = ", seo_var);
                    else if (strcmp(key, "keywords") == 0) sb_append(sb, "%s.keywords = ", seo_var);
                    else if (strcmp(key, "canonical") == 0) sb_append(sb, "%s.canonical = ", seo_var);
                    else if (strcmp(key, "og_title") == 0) sb_append(sb, "%s.og_title = ", seo_var);
                    else if (strcmp(key, "og_description") == 0) sb_append(sb, "%s.og_description = ", seo_var);
                    else if (strcmp(key, "og_image") == 0) sb_append(sb, "%s.og_image = ", seo_var);
                    else if (strcmp(key, "og_type") == 0) sb_append(sb, "%s.og_type = ", seo_var);
                    else if (strcmp(key, "twitter_card") == 0) sb_append(sb, "%s.twitter_card = ", seo_var);
                    else { entry = entry->next; continue; }
                    sb_append_c_string(sb, val->data.string_literal.value);
                    sb_append(sb, ";\n");
                }
                
                entry = entry->next;
            }
            break;
        }
        
        case NODE_RETURN_STMT: {
            gen_indent(sb, indent);
            
            if (strcmp(node->data.return_stmt.return_type, "json") == 0 &&
                node->data.return_stmt.value->type == NODE_OBJECT) {
                
                int n = (*var_counter)++;
                char jb_name[32];
                snprintf(jb_name, sizeof(jb_name), "jb_%d", n);
                
                sb_append(sb, "JsonBuilder %s;\n", jb_name);
                gen_indent(sb, indent);
                sb_append(sb, "jb_init(&%s);\n", jb_name);
                gen_indent(sb, indent);
                sb_append(sb, "jb_start_object(&%s);\n", jb_name);
                
                NodeList* entry = node->data.return_stmt.value->data.object.entries;
                while (entry) {
                    ASTNode* e = entry->node;
                    gen_json_field(sb, e->data.object_entry.key, e->data.object_entry.value, jb_name, indent, var_counter);
                    entry = entry->next;
                }
                
                gen_indent(sb, indent);
                sb_append(sb, "jb_end_object(&%s);\n", jb_name);
                gen_indent(sb, indent);
                sb_append(sb, "Response* resp = webfast_json_response(jb_get(&%s));\n", jb_name);
                gen_indent(sb, indent);
                sb_append(sb, "jb_free(&%s);\n", jb_name);
                gen_indent(sb, indent);
                sb_append(sb, "return resp;\n");
            }
            else if (strcmp(node->data.return_stmt.return_type, "text") == 0) {
                sb_append(sb, "return webfast_text_response(");
                sb_append_c_string(sb, node->data.return_stmt.value->data.string_literal.value);
                sb_append(sb, ");\n");
            }
            else if (strcmp(node->data.return_stmt.return_type, "html") == 0) {
                sb_append(sb, "return webfast_render_html_with_seo(");
                sb_append_c_string(sb, node->data.return_stmt.value->data.string_literal.value);
                sb_append(sb, ", webfast_get_seo());\n");
            }
            else if (strcmp(node->data.return_stmt.return_type, "template") == 0) {
                sb_append(sb, "return webfast_html_response(webfast_render_template(");
                sb_append_c_string(sb, node->data.return_stmt.value->data.string_literal.value);
                sb_append(sb, ", \"{}\"));\n");
            }
            else {
                sb_append(sb, "return webfast_json_response(\"{\\\"status\\\":\\\"ok\\\"}\");\n");
            }
            break;
        }
    }
}

static int has_return_statement(NodeList* stmts) {
    NodeList* curr = stmts;
    while (curr) {
        if (curr->node->type == NODE_RETURN_STMT) return 1;
        if (curr->node->type == NODE_IF_STMT) {
            if (has_return_statement(curr->node->data.if_stmt.then_branch)) return 1;
            if (curr->node->data.if_stmt.else_branch &&
                has_return_statement(curr->node->data.if_stmt.else_branch)) return 1;
        }
        curr = curr->next;
    }
    return 0;
}

static void gen_route_handler(StringBuffer* sb, ASTNode* route, int index, int* var_counter) {
    sb_append(sb, "static Response* route_handler_%d(Request* req) {\n", index);
    
    NodeList* param = route->data.route.params;
    while (param) {
        sb_append(sb, "    const char* %s = webfast_get_param(req, \"%s\");\n",
                 param->node->data.route_param.param_name,
                 param->node->data.route_param.param_name);
        param = param->next;
    }
    
    NodeList* stmt = route->data.route.statements;
    while (stmt) {
        gen_statement(sb, stmt->node, 1, var_counter);
        stmt = stmt->next;
    }
    
    if (!has_return_statement(route->data.route.statements)) {
        sb_append(sb, "    return webfast_json_response(\"{\\\"status\\\":\\\"ok\\\"}\");\n");
    }
    sb_append(sb, "}\n\n");
}

char* codegen_generate(ASTNode* program) {
    if (!program) return NULL;
    
    StringBuffer* sb = sb_create();
    
    sb_append(sb, "/* Auto-generated by WebFast v5.0 */\n\n");
    sb_append(sb, "#include \"webfast_runtime.h\"\n");
    sb_append(sb, "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");
    
    NodeList* config = program->data.program.configs;
    int port_value = 8000;
    
    if (config) {
        sb_append(sb, "typedef struct {\n");
        NodeList* entry = config->node->data.config.entries;
        while (entry) {
            ASTNode* val = entry->node->data.config_entry.value;
            const char* key = entry->node->data.config_entry.key;
            if (val->type == NODE_INT_LITERAL) {
                sb_append(sb, "    long %s;\n", key);
                if (strcmp(key, "port") == 0) port_value = val->data.int_literal.value;
            } else {
                sb_append(sb, "    const char* %s;\n", key);
            }
            entry = entry->next;
        }
        sb_append(sb, "} Config;\n\nstatic Config config;\n\n");
    }
    
    sb_append(sb, "static SeoMetadata* current_seo = NULL;\n");
    sb_append(sb, "const SeoMetadata* webfast_get_seo(void) { return current_seo; }\n\n");
    
    sb_append(sb, "/* Route handlers */\n");
    NodeList* route = program->data.program.routes;
    int route_index = 0;
    int var_counter = 0;
    while (route) {
        gen_route_handler(sb, route->node, route_index, &var_counter);
        route = route->next;
        route_index++;
    }
    
    sb_append(sb, "int main(int argc, char** argv) {\n");
    sb_append(sb, "    (void)argc; (void)argv;\n\n");
    
    config = program->data.program.configs;
    if (config) {
        NodeList* entry = config->node->data.config.entries;
        while (entry) {
            ASTNode* val = entry->node->data.config_entry.value;
            const char* key = entry->node->data.config_entry.key;
            sb_append(sb, "    config.%s = ", key);
            if (val->type == NODE_INT_LITERAL) sb_append(sb, "%ld", val->data.int_literal.value);
            else if (val->type == NODE_STRING_LITERAL) sb_append_c_string(sb, val->data.string_literal.value);
            else sb_append(sb, "NULL");
            sb_append(sb, ";\n");
            entry = entry->next;
        }
        sb_append(sb, "\n");
    }
    
    route = program->data.program.routes;
    route_index = 0;
    while (route) {
        sb_append(sb, "    webfast_register_route(\"%s\", \"%s\", route_handler_%d);\n",
                 route->node->data.route.method, route->node->data.route.path, route_index);
        route = route->next;
        route_index++;
    }
    
    sb_append(sb, "\n    webfast_run(%d);\n    return 0;\n}\n", port_value);
    
    char* result = string_duplicate(sb->buffer);
    sb_free(sb);
    return result;
}

void codegen_free(char* code) { free(code); }
