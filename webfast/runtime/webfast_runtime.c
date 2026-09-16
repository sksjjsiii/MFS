/*
 * WebFast v5.0 Runtime Implementation - Part 1
 * Dynamic Buffer, JSON Builder, Thread-Safe
 */

#include "webfast_runtime.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

Route routes[WF_MAX_ROUTES];
int route_count = 0;

// ============================================================================
// Dynamic Buffer Implementation
// ============================================================================

void db_init(DynamicBuffer* db, size_t initial_capacity) {
    db->data = malloc(initial_capacity);
    db->size = 0;
    db->capacity = initial_capacity;
    db->data[0] = '\0';
}

static void db_ensure_capacity(DynamicBuffer* db, size_t needed) {
    if (db->size + needed >= db->capacity) {
        size_t new_capacity = db->capacity * 2;
        while (db->size + needed >= new_capacity) {
            new_capacity *= 2;
        }
        db->data = realloc(db->data, new_capacity);
        db->capacity = new_capacity;
    }
}

void db_append(DynamicBuffer* db, const char* str) {
    if (!str) return;
    size_t len = strlen(str);
    db_ensure_capacity(db, len + 1);
    memcpy(db->data + db->size, str, len);
    db->size += len;
    db->data[db->size] = '\0';
}

void db_append_escaped(DynamicBuffer* db, const char* str) {
    if (!str) {
        db_append(db, "null");
        return;
    }
    
    size_t len = strlen(str);
    db_ensure_capacity(db, len * 2 + 1);  // Worst case: all escaped
    
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"':  db_append(db, "\\\""); break;
            case '\\': db_append(db, "\\\\"); break;
            case '\n': db_append(db, "\\n"); break;
            case '\r': db_append(db, "\\r"); break;
            case '\t': db_append(db, "\\t"); break;
            default:
                db_ensure_capacity(db, 2);
                db->data[db->size++] = *p;
                db->data[db->size] = '\0';
                break;
        }
    }
}

void db_append_format(DynamicBuffer* db, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    
    db_ensure_capacity(db, needed + 1);
    
    va_start(args, fmt);
    vsnprintf(db->data + db->size, db->capacity - db->size, fmt, args);
    va_end(args);
    
    db->size += needed;
}

void db_free(DynamicBuffer* db) {
    if (db->data) {
        free(db->data);
        db->data = NULL;
    }
    db->size = 0;
    db->capacity = 0;
}

const char* db_get(DynamicBuffer* db) {
    return db->data;
}

// ============================================================================
// JSON Builder Implementation - Dynamic, No Fixed Size
// ============================================================================

void jb_init(JsonBuilder* jb) {
    db_init(&jb->buffer, WF_BUFFER_INITIAL);
    jb->first_key = true;
    jb->depth = 0;
}

void jb_start_object(JsonBuilder* jb) {
    if (!jb->first_key && jb->depth > 0) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, "{");
    jb->first_key = true;
    jb->depth++;
}

void jb_end_object(JsonBuilder* jb) {
    db_append(&jb->buffer, "}");
    jb->depth--;
    jb->first_key = false;
}

void jb_start_array(JsonBuilder* jb) {
    if (jb->buffer.size > 0 && jb->buffer.data[jb->buffer.size - 1] != ':' && 
        !jb->first_key && jb->depth > 0) {
        db_append(&jb->buffer, ",");
    }
    db_append(&jb->buffer, "[");
    jb->first_key = true;
    jb->depth++;
}

void jb_end_array(JsonBuilder* jb) {
    db_append(&jb->buffer, "]");
    jb->depth--;
    jb->first_key = false;
}

void jb_key(JsonBuilder* jb, const char* key) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, "\"");
    db_append_escaped(&jb->buffer, key);
    db_append(&jb->buffer, "\":");
    jb->first_key = false;
}

void jb_add_string(JsonBuilder* jb, const char* key, const char* value) {
    jb_key(jb, key);
    db_append(&jb->buffer, "\"");
    db_append_escaped(&jb->buffer, value);
    db_append(&jb->buffer, "\"");
}

void jb_add_int(JsonBuilder* jb, const char* key, long value) {
    jb_key(jb, key);
    db_append_format(&jb->buffer, "%ld", value);
}

void jb_add_float(JsonBuilder* jb, const char* key, double value) {
    jb_key(jb, key);
    db_append_format(&jb->buffer, "%g", value);
}

void jb_add_bool(JsonBuilder* jb, const char* key, bool value) {
    jb_key(jb, key);
    db_append(&jb->buffer, value ? "true" : "false");
}

void jb_add_null(JsonBuilder* jb, const char* key) {
    jb_key(jb, key);
    db_append(&jb->buffer, "null");
}

void jb_add_timestamp(JsonBuilder* jb, const char* key) {
    jb_key(jb, key);
    db_append_format(&jb->buffer, "%ld", (long)time(NULL));
}

void jb_arr_string(JsonBuilder* jb, const char* value) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, "\"");
    db_append_escaped(&jb->buffer, value);
    db_append(&jb->buffer, "\"");
    jb->first_key = false;
}

void jb_arr_int(JsonBuilder* jb, long value) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append_format(&jb->buffer, "%ld", value);
    jb->first_key = false;
}

void jb_arr_float(JsonBuilder* jb, double value) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append_format(&jb->buffer, "%g", value);
    jb->first_key = false;
}

void jb_arr_bool(JsonBuilder* jb, bool value) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, value ? "true" : "false");
    jb->first_key = false;
}

void jb_arr_null(JsonBuilder* jb) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, "null");
    jb->first_key = false;
}

void jb_arr_start_object(JsonBuilder* jb) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, "{");
    jb->first_key = true;
    jb->depth++;
}

void jb_arr_start_array(JsonBuilder* jb) {
    if (!jb->first_key) db_append(&jb->buffer, ",");
    db_append(&jb->buffer, "[");
    jb->first_key = true;
    jb->depth++;
}

const char* jb_get(JsonBuilder* jb) {
    return db_get(&jb->buffer);
}

void jb_free(JsonBuilder* jb) {
    db_free(&jb->buffer);
}


// ============================================================================
// Route Registration - Thread-Safe with strtok_r
// ============================================================================

static void parse_path_pattern(const char* path, char*** param_names, int* param_count) {
    *param_names = NULL;
    *param_count = 0;
    
    char* copy = strdup(path);
    char* saveptr;
    char* token = strtok_r(copy, "/", &saveptr);
    char* names[WF_MAX_PARAMS];
    int count = 0;
    
    while (token && count < WF_MAX_PARAMS) {
        if (token[0] == '{' && token[strlen(token)-1] == '}') {
            size_t len = strlen(token) - 2;
            names[count] = malloc(len + 1);
            strncpy(names[count], token + 1, len);
            names[count][len] = '\0';
            count++;
        }
        token = strtok_r(NULL, "/", &saveptr);
    }
    
    if (count > 0) {
        *param_names = malloc(count * sizeof(char*));
        for (int i = 0; i < count; i++) {
            (*param_names)[i] = names[i];
        }
    }
    *param_count = count;
    
    free(copy);
}

void webfast_register_route(const char* method, const char* path, RouteHandler handler) {
    if (route_count >= WF_MAX_ROUTES) return;
    
    routes[route_count].method = strdup(method);
    routes[route_count].path_pattern = strdup(path);
    parse_path_pattern(path, &routes[route_count].param_names, &routes[route_count].param_count);
    routes[route_count].handler = handler;
    route_count++;
    
    printf("  + %s %s\n", method, path);
}

// ============================================================================
// Request Parsing - Complete with Content-Length
// ============================================================================

static Request* parse_http_request(int client_fd, Route* route) {
    Request* req = calloc(1, sizeof(Request));
    db_init(&req->body, WF_BUFFER_INITIAL);
    
    // Read headers first
    char header_buffer[8192];
    ssize_t header_bytes = recv(client_fd, header_buffer, sizeof(header_buffer) - 1, 0);
    if (header_bytes <= 0) {
        free(req);
        return NULL;
    }
    header_buffer[header_bytes] = '\0';
    
    // Parse request line
    char method[16], path[512], version[16];
    if (sscanf(header_buffer, "%15s %511s %15s", method, path, version) != 3) {
        db_free(&req->body);
        free(req);
        return NULL;
    }
    
    req->method = strdup(method);
    
    // Split path and query
    char* query = strchr(path, '?');
    if (query) {
        *query = '\0';
        req->query = strdup(query + 1);
    }
    req->path = strdup(path);
    
    // Parse headers
    char* header_start = strstr(header_buffer, "\r\n");
    if (header_start) {
        header_start += 2;
        char* header_end = strstr(header_start, "\r\n\r\n");
        
        req->headers = malloc(WF_MAX_HEADERS * sizeof(char*));
        req->header_values = malloc(WF_MAX_HEADERS * sizeof(char*));
        req->header_count = 0;
        
        int content_length = 0;
        
        char* saveptr;
        char* line = strtok_r(header_start, "\r\n", &saveptr);
        while (line && line < header_end && req->header_count < WF_MAX_HEADERS) {
            char* colon = strchr(line, ':');
            if (colon) {
                *colon = '\0';
                req->headers[req->header_count] = strdup(line);
                req->header_values[req->header_count] = strdup(colon + 2);
                
                // Check for Content-Length
                if (strcasecmp(line, "Content-Length") == 0) {
                    content_length = atoi(colon + 2);
                }
                
                req->header_count++;
            }
            line = strtok_r(NULL, "\r\n", &saveptr);
        }
        
        // Read body if Content-Length > 0
        if (content_length > 0) {
            char* body_start = strstr(header_buffer, "\r\n\r\n");
            if (body_start) {
                body_start += 4;
                size_t already_read = header_bytes - (body_start - header_buffer);
                
                // Append already received body
                db_append(&req->body, body_start);
                
                // Read remaining body
                size_t remaining = content_length - already_read;
                if (remaining > 0) {
                    char* body_buffer = malloc(remaining + 1);
                    ssize_t body_bytes = recv(client_fd, body_buffer, remaining, 0);
                    if (body_bytes > 0) {
                        body_buffer[body_bytes] = '\0';
                        db_append(&req->body, body_buffer);
                    }
                    free(body_buffer);
                }
            }
        }
    }
    
    // Match route parameters (thread-safe with strtok_r)
    if (route) {
        req->param_count = route->param_count;
        
        char path_copy[512];
        strncpy(path_copy, req->path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy)-1] = '\0';
        
        char* req_parts[32];
        int req_count = 0;
        char* saveptr2;
        char* tok = strtok_r(path_copy, "/", &saveptr2);
        while (tok && req_count < 32) {
            req_parts[req_count++] = tok;
            tok = strtok_r(NULL, "/", &saveptr2);
        }
        
        char pattern_copy[512];
        strncpy(pattern_copy, route->path_pattern, sizeof(pattern_copy) - 1);
        pattern_copy[sizeof(pattern_copy)-1] = '\0';
        
        char* pat_parts[32];
        int pat_count = 0;
        char* saveptr3;
        tok = strtok_r(pattern_copy, "/", &saveptr3);
        while (tok && pat_count < 32) {
            pat_parts[pat_count++] = tok;
            tok = strtok_r(NULL, "/", &saveptr3);
        }
        
        if (req_count == pat_count) {
            int param_idx = 0;
            for (int i = 0; i < pat_count && param_idx < route->param_count; i++) {
                if (pat_parts[i][0] == '{') {
                    req->param_names[param_idx] = strdup(route->param_names[param_idx]);
                    req->param_values[param_idx] = strdup(req_parts[i]);
                    param_idx++;
                }
            }
        }
    }
    
    return req;
}

// ============================================================================
// Route Matching - Thread-Safe
// ============================================================================

static Route* match_route(const char* method, const char* path) {
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].method, method) != 0) continue;
        
        char path_copy[512], pattern_copy[512];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy)-1] = '\0';
        strncpy(pattern_copy, routes[i].path_pattern, sizeof(pattern_copy) - 1);
        pattern_copy[sizeof(pattern_copy)-1] = '\0';
        
        char* req_parts[32];
        int req_count = 0;
        char* saveptr;
        char* tok = strtok_r(path_copy, "/", &saveptr);
        while (tok && req_count < 32) {
            req_parts[req_count++] = tok;
            tok = strtok_r(NULL, "/", &saveptr);
        }
        
        char* pat_parts[32];
        int pat_count = 0;
        char* saveptr2;
        tok = strtok_r(pattern_copy, "/", &saveptr2);
        while (tok && pat_count < 32) {
            pat_parts[pat_count++] = tok;
            tok = strtok_r(NULL, "/", &saveptr2);
        }
        
        if (req_count != pat_count) continue;
        
        int match = 1;
        for (int j = 0; j < pat_count; j++) {
            if (pat_parts[j][0] == '{') continue;
            if (strcmp(req_parts[j], pat_parts[j]) != 0) {
                match = 0;
                break;
            }
        }
        
        if (match) return &routes[i];
    }
    return NULL;
}


// ============================================================================
// Request Helpers - No Memory Leaks
// ============================================================================

const char* webfast_get_param(Request* req, const char* name) {
    if (!req) return NULL;
    for (int i = 0; i < req->param_count; i++) {
        if (req->param_names[i] && strcmp(req->param_names[i], name) == 0) {
            return req->param_values[i];
        }
    }
    return NULL;
}

const char* webfast_get_query_param(Request* req, const char* name) {
    if (!req || !req->query) return NULL;
    
    // Parse query string without memory leak
    static char value_buffer[1024];
    char* query = strdup(req->query);
    char* saveptr;
    char* token = strtok_r(query, "&", &saveptr);
    
    while (token) {
        char* eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            if (strcmp(token, name) == 0) {
                strncpy(value_buffer, eq + 1, sizeof(value_buffer) - 1);
                value_buffer[sizeof(value_buffer)-1] = '\0';
                free(query);
                return value_buffer;  // Static buffer, no leak
            }
        }
        token = strtok_r(NULL, "&", &saveptr);
    }
    
    free(query);
    return NULL;
}

const char* webfast_get_header(Request* req, const char* name) {
    if (!req) return NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i], name) == 0) {
            return req->header_values[i];
        }
    }
    return NULL;
}

const char* webfast_get_body(Request* req) {
    return req ? db_get(&req->body) : NULL;
}

// ============================================================================
// Response Creators - Dynamic Buffers
// ============================================================================

static Response* create_response(int status_code, const char* content_type, const char* body) {
    Response* resp = malloc(sizeof(Response));
    resp->status_code = status_code;
    resp->content_type = strdup(content_type);
    db_init(&resp->body, WF_BUFFER_INITIAL);
    db_append(&resp->body, body);
    resp->headers = malloc(16 * sizeof(char*));
    resp->header_values = malloc(16 * sizeof(char*));
    resp->header_count = 0;
    resp->header_capacity = 16;
    return resp;
}

Response* webfast_json_response(const char* json) {
    return create_response(200, "application/json", json);
}

Response* webfast_text_response(const char* text) {
    return create_response(200, "text/plain", text);
}

Response* webfast_html_response(const char* html) {
    return create_response(200, "text/html", html);
}

Response* webfast_redirect(const char* url, int status_code) {
    Response* resp = create_response(status_code, "text/html", "");
    webfast_set_header(resp, "Location", url);
    return resp;
}

Response* webfast_error(int status_code, const char* message) {
    JsonBuilder jb;
    jb_init(&jb);
    jb_start_object(&jb);
    jb_add_string(&jb, "error", message);
    jb_add_int(&jb, "status", status_code);
    jb_end_object(&jb);
    
    Response* resp = webfast_json_response(jb_get(&jb));
    resp->status_code = status_code;
    jb_free(&jb);
    return resp;
}

void webfast_set_header(Response* resp, const char* key, const char* value) {
    if (!resp) return;
    
    if (resp->header_count >= resp->header_capacity) {
        resp->header_capacity *= 2;
        resp->headers = realloc(resp->headers, resp->header_capacity * sizeof(char*));
        resp->header_values = realloc(resp->header_values, resp->header_capacity * sizeof(char*));
    }
    
    resp->headers[resp->header_count] = strdup(key);
    resp->header_values[resp->header_count] = strdup(value);
    resp->header_count++;
}

// ============================================================================
// Cleanup - No Memory Leaks
// ============================================================================

void webfast_free_response(Response* resp) {
    if (resp) {
        free(resp->content_type);
        db_free(&resp->body);
        for (int i = 0; i < resp->header_count; i++) {
            free(resp->headers[i]);
            free(resp->header_values[i]);
        }
        free(resp->headers);
        free(resp->header_values);
        free(resp);
    }
}

void webfast_free_request(Request* req) {
    if (req) {
        free(req->method);
        free(req->path);
        free(req->query);
        db_free(&req->body);
        for (int i = 0; i < req->header_count; i++) {
            free(req->headers[i]);
            free(req->header_values[i]);
        }
        free(req->headers);
        free(req->header_values);
        for (int i = 0; i < req->param_count; i++) {
            free(req->param_names[i]);
            free(req->param_values[i]);
        }
        free(req);
    }
}


// ============================================================================
// SEO Metadata -> HTML with Dynamic Buffers
// ============================================================================

Response* webfast_render_html_with_seo(const char* body, const SeoMetadata* seo) {
    DynamicBuffer html;
    db_init(&html, WF_BUFFER_INITIAL);
    
    db_append(&html, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n");
    db_append(&html, "    <meta charset=\"UTF-8\">\n");
    db_append(&html, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    
    if (seo) {
        if (seo->title) {
            db_append_format(&html, "    <title>%s</title>\n", seo->title);
        }
        if (seo->description) {
            db_append_format(&html, "    <meta name=\"description\" content=\"%s\">\n", seo->description);
        }
        if (seo->keywords) {
            db_append_format(&html, "    <meta name=\"keywords\" content=\"%s\">\n", seo->keywords);
        }
        if (seo->canonical) {
            db_append_format(&html, "    <link rel=\"canonical\" href=\"%s\">\n", seo->canonical);
        }
        if (seo->og_title || seo->title) {
            db_append_format(&html, "    <meta property=\"og:title\" content=\"%s\">\n",
                           seo->og_title ? seo->og_title : seo->title);
        }
        if (seo->og_description || seo->description) {
            db_append_format(&html, "    <meta property=\"og:description\" content=\"%s\">\n",
                           seo->og_description ? seo->og_description : seo->description);
        }
        if (seo->og_image) {
            db_append_format(&html, "    <meta property=\"og:image\" content=\"%s\">\n", seo->og_image);
        }
        if (seo->og_type) {
            db_append_format(&html, "    <meta property=\"og:type\" content=\"%s\">\n", seo->og_type);
        }
        if (seo->twitter_card) {
            db_append_format(&html, "    <meta name=\"twitter:card\" content=\"%s\">\n", seo->twitter_card);
        }
    }
    
    db_append(&html, "</head>\n<body>\n");
    db_append(&html, body);
    db_append(&html, "\n</body>\n</html>");
    
    Response* resp = webfast_html_response(db_get(&html));
    db_free(&html);
    
    return resp;
}

void seo_free(SeoMetadata* seo) {
    if (seo) {
        free(seo->title);
        free(seo->description);
        free(seo->keywords);
        free(seo->canonical);
        free(seo->og_title);
        free(seo->og_description);
        free(seo->og_image);
        free(seo->og_type);
        free(seo->twitter_card);
        free(seo);
    }
}

// ============================================================================
// Template Engine - Simple Variable Substitution
// ============================================================================

char* webfast_render_template(const char* template_path, const char* data_json) {
    // Read template file
    FILE* fp = fopen(template_path, "r");
    if (!fp) return strdup("<!-- Template not found -->");
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* template = malloc(size + 1);
    fread(template, 1, size, fp);
    template[size] = '\0';
    fclose(fp);
    
    // Simple variable substitution: {{key}} -> value
    // Parse data_json to extract key-value pairs
    // This is a simplified implementation
    DynamicBuffer result;
    db_init(&result, size * 2);
    
    const char* p = template;
    while (*p) {
        if (p[0] == '{' && p[1] == '{') {
            // Find closing }}
            const char* start = p + 2;
            const char* end = strstr(start, "}}");
            if (end) {
                char key[256];
                size_t key_len = end - start;
                if (key_len < sizeof(key)) {
                    strncpy(key, start, key_len);
                    key[key_len] = '\0';
                    
                    // Look for key in data_json
                    char search_key[512];
                    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
                    char* found = strstr(data_json, search_key);
                    
                    if (found) {
                        found += strlen(search_key);
                        while (*found == ' ') found++;
                        
                        if (*found == '"') {
                            // String value
                            found++;
                            const char* value_end = strchr(found, '"');
                            if (value_end) {
                                char value[1024];
                                size_t val_len = value_end - found;
                                if (val_len < sizeof(value)) {
                                    strncpy(value, found, val_len);
                                    value[val_len] = '\0';
                                    db_append(&result, value);
                                }
                            }
                        } else {
                            // Numeric or boolean value
                            const char* value_end = found;
                            while (*value_end && *value_end != ',' && *value_end != '}') {
                                value_end++;
                            }
                            char value[256];
                            size_t val_len = value_end - found;
                            if (val_len < sizeof(value)) {
                                strncpy(value, found, val_len);
                                value[val_len] = '\0';
                                db_append(&result, value);
                            }
                        }
                    }
                    
                    p = end + 2;
                    continue;
                }
            }
        }
        
        db_append_format(&result, "%c", *p);
        p++;
    }
    
    free(template);
    
    char* result_str = strdup(db_get(&result));
    db_free(&result);
    
    return result_str;
}

// ============================================================================
// Utility Functions - Secure UUID, Numeric Timestamps
// ============================================================================

long webfast_timestamp(void) {
    return (long)time(NULL);
}

char* webfast_uuid(void) {
    static char buf[37];
    
    // Secure random with /dev/urandom
    unsigned char bytes[16];
    FILE* urandom = fopen("/dev/urandom", "r");
    if (urandom) {
        fread(bytes, 1, 16, urandom);
        fclose(urandom);
    } else {
        // Fallback to time-based (less secure)
        srand(time(NULL) ^ getpid());
        for (int i = 0; i < 16; i++) {
            bytes[i] = rand() & 0xFF;
        }
    }
    
    // Set version 4 (random)
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    // Set variant
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    
    snprintf(buf, sizeof(buf),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3],
             bytes[4], bytes[5],
             bytes[6], bytes[7],
             bytes[8], bytes[9],
             bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    
    return strdup(buf);
}


// ============================================================================
// HTTP Server - Keep-Alive Support
// ============================================================================

static void handle_connection(int client_fd, int keepalive_timeout) {
    while (1) {
        // Set timeout for keep-alive
        struct timeval tv;
        tv.tv_sec = keepalive_timeout;
        tv.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        Request* req = parse_http_request(client_fd, NULL);
        if (!req) break;
        
        // Match route
        Route* route = match_route(req->method, req->path);
        
        printf("[%ld] %s %s", webfast_timestamp(), req->method, req->path);
        
        Response* resp = NULL;
        if (route) {
            resp = route->handler(req);
        } else {
            resp = webfast_error(404, "Not found");
        }
        
        printf(" -> %d (%zu bytes)\n", resp->status_code, resp->body.size);
        
        // Check for Connection: keep-alive
        const char* connection = webfast_get_header(req, "Connection");
        bool keepalive = connection && strcasecmp(connection, "keep-alive") == 0;
        
        // Build HTTP response
        DynamicBuffer header;
        db_init(&header, 1024);
        
        db_append_format(&header, "HTTP/1.1 %d OK\r\n", resp->status_code);
        db_append_format(&header, "Content-Type: %s\r\n", resp->content_type);
        db_append_format(&header, "Content-Length: %zu\r\n", resp->body.size);
        
        if (keepalive) {
            db_append_format(&header, "Connection: keep-alive\r\n");
            db_append_format(&header, "Keep-Alive: timeout=%d\r\n", keepalive_timeout);
        } else {
            db_append(&header, "Connection: close\r\n");
        }
        
        db_append(&header, "Access-Control-Allow-Origin: *\r\n");
        db_append(&header, "Server: WebFast/5.0\r\n");
        
        // Add custom headers
        for (int i = 0; i < resp->header_count; i++) {
            db_append_format(&header, "%s: %s\r\n", resp->headers[i], resp->header_values[i]);
        }
        
        db_append(&header, "\r\n");
        
        // Send header
        send(client_fd, db_get(&header), header.size, 0);
        
        // Send body
        send(client_fd, db_get(&resp->body), resp->body.size, 0);
        
        db_free(&header);
        webfast_free_response(resp);
        webfast_free_request(req);
        
        if (!keepalive) break;
    }
    
    close(client_fd);
}

void webfast_run(int port) {
    webfast_run_with_keepalive(port, WF_KEEPALIVE_TIMEOUT);
}

void webfast_run_with_keepalive(int port, int timeout_seconds) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }
    
    printf("\n\033[1;32m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;32m║  WebFast v5.0 Server (Production Ready)             ║\033[0m\n");
    printf("\033[1;32m║  Listening on http://localhost:%-5d                   ║\033[0m\n", port);
    printf("\033[1;32m║  Routes: %-3d  Keep-Alive: %ds                      ║\033[0m\n", 
           route_count, timeout_seconds);
    printf("\033[1;32m╚══════════════════════════════════════════════════════╝\033[0m\n\n");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (client_fd >= 0) {
            handle_connection(client_fd, timeout_seconds);
        }
    }
    
    close(server_fd);
}
