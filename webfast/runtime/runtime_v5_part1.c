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

static void jb_key(JsonBuilder* jb, const char* key) {
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
