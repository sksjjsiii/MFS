
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
