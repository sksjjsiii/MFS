
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
