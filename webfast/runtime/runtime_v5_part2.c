
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
