/*
 * WebFast v4.0 Runtime Implementation
 * Complete working implementation with Arrays, For Loops, SEO to HTML
 */

#include "webfast_runtime.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

Route routes[WF_MAX_ROUTES];
int route_count = 0;
SeoMetadata current_seo = {0};

// ============================================================================
// Global SEO Management
// ============================================================================

void webfast_set_seo(const SeoMetadata* seo) {
    if (seo) {
        current_seo = *seo;
    }
}

const SeoMetadata* webfast_get_seo(void) {
    return &current_seo;
}

// ============================================================================
// JSON Builder Implementation - Full Array Support
// ============================================================================

void jb_init(JsonBuilder* jb) {
    jb->pos = 0;
    jb->buffer[0] = '\0';
    jb->first_key = true;
    jb->depth = 0;
}

static void jb_append(JsonBuilder* jb, const char* str) {
    int len = strlen(str);
    if (jb->pos + len < WF_MAX_JSON - 1) {
        memcpy(jb->buffer + jb->pos, str, len);
        jb->pos += len;
        jb->buffer[jb->pos] = '\0';
    }
}

static void jb_append_escaped(JsonBuilder* jb, const char* str) {
    if (!str) {
        jb_append(jb, "null");
        return;
    }
    for (const char* p = str; *p && jb->pos < WF_MAX_JSON - 10; p++) {
        switch (*p) {
            case '"':  jb_append(jb, "\\""); break;
            case '\\': jb_append(jb, "\\\\"); break;
            case '\n': jb_append(jb, "\\n"); break;
            case '\r': jb_append(jb, "\\r"); break;
            case '\t': jb_append(jb, "\\t"); break;
            default:
                jb->buffer[jb->pos++] = *p;
                jb->buffer[jb->pos] = '\0';
                break;
        }
    }
}

void jb_start_object(JsonBuilder* jb) {
    if (!jb->first_key && jb->depth > 0) jb_append(jb, ",");
    jb_append(jb, "{");
    jb->first_key = true;
    jb->depth++;
}

void jb_end_object(JsonBuilder* jb) {
    jb_append(jb, "}");
    jb->depth--;
    jb->first_key = false;
}

void jb_start_array(JsonBuilder* jb) {
    if (!jb->first_key && jb->depth > 0) jb_append(jb, ",");
    jb_append(jb, "[");
    jb->first_key = true;
    jb->depth++;
}

void jb_end_array(JsonBuilder* jb) {
    jb_append(jb, "]");
    jb->depth--;
    jb->first_key = false;
}

static void jb_key(JsonBuilder* jb, const char* key) {
    if (!jb->first_key) jb_append(jb, ",");
    jb_append(jb, "\"");
    jb_append_escaped(jb, key);
    jb_append(jb, "\":");
    jb->first_key = false;
}

void jb_add_string(JsonBuilder* jb, const char* key, const char* value) {
    jb_key(jb, key);
    jb_append(jb, "\"");
    jb_append_escaped(jb, value);
    jb_append(jb, "\"");
}

void jb_add_int(JsonBuilder* jb, const char* key, long value) {
    jb_key(jb, key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", value);
    jb_append(jb, buf);
}

void jb_add_float(JsonBuilder* jb, const char* key, double value) {
    jb_key(jb, key);
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    jb_append(jb, buf);
}

void jb_add_bool(JsonBuilder* jb, const char* key, bool value) {
    jb_key(jb, key);
    jb_append(jb, value ? "true" : "false");
}

void jb_add_null(JsonBuilder* jb, const char* key) {
    jb_key(jb, key);
    jb_append(jb, "null");
}

void jb_add_raw(JsonBuilder* jb, const char* key, const char* raw_json) {
    jb_key(jb, key);
    jb_append(jb, raw_json ? raw_json : "null");
}

// Array element methods (no key - used inside arrays)
void jb_arr_string(JsonBuilder* jb, const char* value) {
    if (!jb->first_key) jb_append(jb, ",");
    jb_append(jb, "\"");
    jb_append_escaped(jb, value);
    jb_append(jb, "\"");
    jb->first_key = false;
}

void jb_arr_int(JsonBuilder* jb, long value) {
    if (!jb->first_key) jb_append(jb, ",");
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", value);
    jb_append(jb, buf);
    jb->first_key = false;
}

void jb_arr_bool(JsonBuilder* jb, bool value) {
    if (!jb->first_key) jb_append(jb, ",");
    jb_append(jb, value ? "true" : "false");
    jb->first_key = false;
}

void jb_arr_null(JsonBuilder* jb) {
    if (!jb->first_key) jb_append(jb, ",");
    jb_append(jb, "null");
    jb->first_key = false;
}

void jb_arr_start_object(JsonBuilder* jb) {
    if (!jb->first_key) jb_append(jb, ",");
    jb_append(jb, "{");
    jb->first_key = true;
    jb->depth++;
}

void jb_arr_start_array(JsonBuilder* jb) {
    if (!jb->first_key) jb_append(jb, ",");
    jb_append(jb, "[");
    jb->first_key = true;
    jb->depth++;
}

const char* jb_get(JsonBuilder* jb) {
    return jb->buffer;
}

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
