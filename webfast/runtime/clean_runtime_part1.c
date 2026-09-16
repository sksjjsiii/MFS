/*
 * WebFast v4.0 Runtime - Complete Clean Implementation
 * Supports: Arrays, For Loops, SEO to HTML, High Performance
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
            case '"':  
                jb->buffer[jb->pos++] = '\\';
                jb->buffer[jb->pos++] = '"';
                jb->buffer[jb->pos] = '\0';
                break;
            case '\\': 
                jb->buffer[jb->pos++] = '\\';
                jb->buffer[jb->pos++] = '\\';
                jb->buffer[jb->pos] = '\0';
                break;
            case '\n': 
                jb->buffer[jb->pos++] = '\\';
                jb->buffer[jb->pos++] = 'n';
                jb->buffer[jb->pos] = '\0';
                break;
            case '\r': 
                jb->buffer[jb->pos++] = '\\';
                jb->buffer[jb->pos++] = 'r';
                jb->buffer[jb->pos] = '\0';
                break;
            case '\t': 
                jb->buffer[jb->pos++] = '\\';
                jb->buffer[jb->pos++] = 't';
                jb->buffer[jb->pos] = '\0';
                break;
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
