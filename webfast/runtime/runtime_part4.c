/*
 * WebFast v4.0 Runtime Implementation - Part 4
 * High-Performance HTTP Server (epoll-based)
 */

// ============================================================================
// HTTP Server Implementation
// ============================================================================

static Request* parse_http_request(const char* raw, size_t len, Route* route) {
    Request* req = calloc(1, sizeof(Request));
    
    // Parse request line
    char* line_end = strstr(raw, "\r\n");
    if (!line_end) {
        free(req);
        return NULL;
    }
    
    char request_line[1024];
    size_t line_len = line_end - raw;
    strncpy(request_line, raw, line_len);
    request_line[line_len] = '\0';
    
    char method[16], path[512], version[16];
    if (sscanf(request_line, "%15s %511s %15s", method, path, version) != 3) {
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
    const char* header_start = line_end + 2;
    const char* header_end = strstr(header_start, "\r\n\r\n");
    
    if (header_end) {
        req->headers = malloc(WF_MAX_HEADERS * sizeof(char*));
        req->header_values = malloc(WF_MAX_HEADERS * sizeof(char*));
        req->header_count = 0;
        
        const char* current = header_start;
        while (current < header_end && req->header_count < WF_MAX_HEADERS) {
            const char* next_line = strstr(current, "\r\n");
            if (!next_line) break;
            
            char header_line[1024];
            size_t hlen = next_line - current;
            strncpy(header_line, current, hlen);
            header_line[hlen] = '\0';
            
            char* colon = strchr(header_line, ':');
            if (colon) {
                *colon = '\0';
                req->headers[req->header_count] = strdup(header_line);
                req->header_values[req->header_count] = strdup(colon + 2);
                req->header_count++;
            }
            
            current = next_line + 2;
        }
        
        // Parse cookies from Cookie header
        const char* cookie_header = webfast_get_header(req, "Cookie");
        if (cookie_header) {
            char* cookie_copy = strdup(cookie_header);
            char* cookie_token = strtok(cookie_copy, ";");
            
            while (cookie_token && req->cookie_count < WF_MAX_PARAMS) {
                while (*cookie_token == ' ') cookie_token++;
                
                char* eq = strchr(cookie_token, '=');
                if (eq) {
                    *eq = '\0';
                    req->cookies[req->cookie_count] = strdup(cookie_token);
                    req->cookie_values[req->cookie_count] = strdup(eq + 1);
                    req->cookie_count++;
                }
                
                cookie_token = strtok(NULL, ";");
            }
            
            free(cookie_copy);
        }
        
        // Parse body
        const char* body_start = header_end + 4;
        if (body_start < raw + len) {
            req->body_length = len - (body_start - raw);
            req->body = malloc(req->body_length + 1);
            memcpy(req->body, body_start, req->body_length);
            req->body[req->body_length] = '\0';
        }
    }
    
    // Match path parameters
    if (route) {
        req->param_count = route->param_count;
        
        char path_copy[512];
        strncpy(path_copy, req->path, sizeof(path_copy) - 1);
        
        char* req_parts[32];
        int req_count = 0;
        char* tok = strtok(path_copy, "/");
        while (tok && req_count < 32) {
            req_parts[req_count++] = tok;
            tok = strtok(NULL, "/");
        }
        
        char pattern_copy[512];
        strncpy(pattern_copy, route->path_pattern, sizeof(pattern_copy) - 1);
        
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
    Request* req = parse_http_request(buffer, n, route);
    
    printf("[%s] %s %s", webfast_now(), method, path);
    
    Response* resp = NULL;
    
    if (route && req) {
        // Execute middleware chain
        RouteHandler handler = route->handler;
        
        // For now, just call the handler directly
        // TODO: Implement full middleware chain
        resp = handler(req);
    } else {
        resp = webfast_error(404, "Not found");
    }
    
    printf(" -> %d (%zu bytes)\n", resp->status_code, resp->body_length);
    
    // Build response
    char header[4096];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Server: WebFast/4.0\r\n"
        "Date: %s\r\n",
        resp->status_code, resp->content_type, resp->body_length, webfast_now());
    
    // Add custom headers
    for (int i = 0; i < resp->header_count; i++) {
        header_len += snprintf(header + header_len, sizeof(header) - header_len,
                              "%s\r\n", resp->headers[i]);
    }
    
    header_len += snprintf(header + header_len, sizeof(header) - header_len, "\r\n");
    
    send(client_fd, header, header_len, 0);
    send(client_fd, resp->body, resp->body_length, 0);
    
    webfast_free_response(resp);
    webfast_free_request(req);
    close(client_fd);
}

static void* worker_thread(void* arg) {
    (void)arg;
    
    struct epoll_event events[64];
    
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, 64, -1);
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                // Accept new connection
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
                
                if (client_fd >= 0) {
                    // Set non-blocking
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                    
                    // Add to epoll
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                }
            } else {
                // Handle client data
                handle_connection(events[i].data.fd);
            }
        }
    }
    
    return NULL;
}

void webfast_run(int port) {
    webfast_run_with_workers(port, WF_WORKER_THREADS);
}

void webfast_run_with_workers(int port, int worker_count) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Set non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }
    
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }
    
    // Create epoll instance
    epoll_fd = epoll_create1(0);
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
    
    printf("\n\033[1;32m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;32m║  WebFast v4.0 - Ultimate Web Framework               ║\033[0m\n");
    printf("\033[1;32m║  Listening on http://localhost:%-5d                   ║\033[0m\n", port);
    printf("\033[1;32m║  Worker threads: %-5d                                 ║\033[0m\n", worker_count);
    printf("\033[1;32m║  Routes registered: %-5d                              ║\033[0m\n", route_count);
    printf("\033[1;32m║  Press Ctrl+C to stop                                ║\033[0m\n");
    printf("\033[1;32m╚══════════════════════════════════════════════════════╝\033[0m\n\n");
    
    // Create worker threads
    pthread_t workers[WF_WORKER_THREADS];
    for (int i = 0; i < worker_count; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }
    
    // Wait for workers
    for (int i = 0; i < worker_count; i++) {
        pthread_join(workers[i], NULL);
    }
    
    close(server_fd);
    close(epoll_fd);
}
