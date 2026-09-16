
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
