
// ============================================================================
// HTTP Server Implementation
// ============================================================================

static Request* parse_http_request(const char* raw, Route* route) {
    Request* req = calloc(1, sizeof(Request));
    
    // Parse method and path
    char method[16], path[512];
    if (sscanf(raw, "%15s %511s", method, path) != 2) {
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
    
    // Match route parameters
    if (route) {
        req->param_count = route->param_count;
        
        char path_copy[512];
        strncpy(path_copy, req->path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy)-1] = '\0';
        
        char* req_parts[32];
        int req_count = 0;
        char* tok = strtok(path_copy, "/");
        while (tok && req_count < 32) {
            req_parts[req_count++] = tok;
            tok = strtok(NULL, "/");
        }
        
        char pattern_copy[512];
        strncpy(pattern_copy, route->path_pattern, sizeof(pattern_copy) - 1);
        pattern_copy[sizeof(pattern_copy)-1] = '\0';
        
        char* pat_parts[32];
        int pat_count = 0;
        tok = strtok(pattern_copy, "/");
        while (tok && pat_count < 32) {
            pat_parts[pat_count++] = tok;
            tok = strtok(NULL, "/");
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
        char* tok = strtok(path_copy, "/");
        while (tok && req_count < 32) {
            req_parts[req_count++] = tok;
            tok = strtok(NULL, "/");
        }
        
        char* pat_parts[32];
        int pat_count = 0;
        tok = strtok(pattern_copy, "/");
        while (tok && pat_count < 32) {
            pat_parts[pat_count++] = tok;
            tok = strtok(NULL, "/");
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

static void handle_connection(int client_fd) {
    char buffer[65536];
    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';
    
    char method[16], path[512];
    if (sscanf(buffer, "%15s %511s", method, path) != 2) {
        close(client_fd);
        return;
    }
    
    char* query = strchr(path, '?');
    char path_only[512];
    strncpy(path_only, path, sizeof(path_only) - 1);
    path_only[sizeof(path_only)-1] = '\0';
    if (query) path_only[query - path] = '\0';
    
    Route* route = match_route(method, path_only);
    Request* req = parse_http_request(buffer, route);
    
    printf("[%s] %s %s", webfast_now(), method, path);
    
    Response* resp = NULL;
    
    if (route && req) {
        resp = route->handler(req);
    } else {
        resp = webfast_error(404, "Not found");
    }
    
    printf(" -> %d (%zu bytes)\n", resp->status_code, resp->body_length);
    
    // Build HTTP response
    char header[4096];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Server: WebFast/4.0\r\n"
        "\r\n",
        resp->status_code, resp->content_type, resp->body_length);
    
    send(client_fd, header, header_len, 0);
    send(client_fd, resp->body, resp->body_length, 0);
    
    webfast_free_response(resp);
    webfast_free_request(req);
    close(client_fd);
}

void webfast_run(int port) {
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
    printf("\033[1;32m║  WebFast v4.0 Server                                 ║\033[0m\n");
    printf("\033[1;32m║  Listening on http://localhost:%-5d                   ║\033[0m\n", port);
    printf("\033[1;32m║  Routes: %-3d  Press Ctrl+C to stop                 ║\033[0m\n", route_count);
    printf("\033[1;32m╚══════════════════════════════════════════════════════╝\033[0m\n\n");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (client_fd >= 0) {
            handle_connection(client_fd);
        }
    }
    
    close(server_fd);
}
