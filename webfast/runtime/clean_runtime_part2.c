
// ============================================================================
// SEO Metadata to HTML
// ============================================================================

char* seo_generate_html_head(const SeoMetadata* seo) {
    if (!seo) return strdup("");
    
    char buf[8192];
    int pos = 0;
    
    pos += snprintf(buf + pos, sizeof(buf) - pos, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n");
    
    if (seo->charset) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta charset=\"%s\">\n", seo->charset);
    } else {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta charset=\"UTF-8\">\n");
    }
    
    pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    
    if (seo->title) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <title>%s</title>\n", seo->title);
    }
    
    if (seo->description) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta name=\"description\" content=\"%s\">\n", seo->description);
    }
    
    if (seo->keywords) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta name=\"keywords\" content=\"%s\">\n", seo->keywords);
    }
    
    if (seo->canonical) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <link rel=\"canonical\" href=\"%s\">\n", seo->canonical);
    }
    
    // Open Graph tags
    if (seo->og_title || seo->title) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta property=\"og:title\" content=\"%s\">\n", 
                       seo->og_title ? seo->og_title : seo->title);
    }
    if (seo->og_description || seo->description) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta property=\"og:description\" content=\"%s\">\n",
                       seo->og_description ? seo->og_description : seo->description);
    }
    if (seo->og_image) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta property=\"og:image\" content=\"%s\">\n", seo->og_image);
    }
    if (seo->og_type) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta property=\"og:type\" content=\"%s\">\n", seo->og_type);
    }
    
    // Twitter Card
    if (seo->twitter_card) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "    <meta name=\"twitter:card\" content=\"%s\">\n", seo->twitter_card);
    }
    
    pos += snprintf(buf + pos, sizeof(buf) - pos, "</head>\n<body>\n");
    
    return strdup(buf);
}

Response* webfast_render_html_with_seo(const char* body, const SeoMetadata* seo) {
    char* head = seo_generate_html_head(seo);
    
    size_t total_len = strlen(head) + strlen(body) + 32;
    char* full_html = malloc(total_len);
    snprintf(full_html, total_len, "%s%s\n</body>\n</html>", head, body);
    
    free(head);
    
    Response* resp = webfast_html_response(full_html);
    free(full_html);
    
    return resp;
}

// ============================================================================
// Route Registration
// ============================================================================

static void parse_path_pattern(const char* path, char*** param_names, int* param_count) {
    *param_names = NULL;
    *param_count = 0;
    
    char* copy = strdup(path);
    char* token = strtok(copy, "/");
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
        token = strtok(NULL, "/");
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
    
    routes[route_count].method = method;
    routes[route_count].path_pattern = path;
    parse_path_pattern(path, &routes[route_count].param_names, &routes[route_count].param_count);
    routes[route_count].handler = handler;
    route_count++;
    
    printf("  + %s %s\n", method, path);
}

// ============================================================================
// Request Helpers
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
    
    char* query = strdup(req->query);
    char* token = strtok(query, "&");
    
    while (token) {
        char* eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            if (strcmp(token, name) == 0) {
                char* value = strdup(eq + 1);
                free(query);
                return value;
            }
        }
        token = strtok(NULL, "&");
    }
    
    free(query);
    return NULL;
}

// ============================================================================
// Response Creators
// ============================================================================

static Response* create_response(int status_code, const char* content_type, const char* body) {
    Response* resp = malloc(sizeof(Response));
    resp->status_code = status_code;
    resp->content_type = content_type;
    resp->body = strdup(body);
    resp->body_length = strlen(body);
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
    return resp;
}

void webfast_free_response(Response* resp) {
    if (resp) {
        free(resp->body);
        free(resp);
    }
}

void webfast_free_request(Request* req) {
    if (req) {
        for (int i = 0; i < req->param_count; i++) {
            free(req->param_names[i]);
            free(req->param_values[i]);
        }
        free(req);
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* webfast_now(void) {
    static char buf[64];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

long webfast_timestamp(void) {
    return (long)time(NULL);
}

char* webfast_uuid(void) {
    static char buf[37];
    srand(time(NULL));
    snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012x",
             rand(), rand() & 0xFFFF, (rand() & 0x0FFF) | 0x4000,
             (rand() & 0x3FFF) | 0x8000, rand());
    return strdup(buf);
}
