/*
 * WebFast v5.0 Runtime - Production Ready
 * All Critical Issues Fixed:
 * 1. Dynamic memory management (no fixed 64KB buffer)
 * 2. Thread-safe strtok_r
 * 3. No memory leaks
 * 4. Complete HTTP parsing with Content-Length
 * 5. Secure UUID with /dev/urandom
 * 6. Keep-alive support
 * 7. Numeric timestamps
 */

#ifndef WEBFAST_RUNTIME_H
#define WEBFAST_RUNTIME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define WF_MAX_PARAMS 32
#define WF_MAX_HEADERS 64
#define WF_MAX_ROUTES 2000
#define WF_BUFFER_INITIAL 4096
#define WF_KEEPALIVE_TIMEOUT 30

// ============================================================================
// Dynamic Buffer - Grows as needed
// ============================================================================

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} DynamicBuffer;

void db_init(DynamicBuffer* db, size_t initial_capacity);
void db_append(DynamicBuffer* db, const char* str);
void db_append_escaped(DynamicBuffer* db, const char* str);
void db_append_format(DynamicBuffer* db, const char* fmt, ...);
void db_free(DynamicBuffer* db);
const char* db_get(DynamicBuffer* db);

// ============================================================================
// Core Data Types
// ============================================================================

typedef struct {
    int status_code;
    char* content_type;
    DynamicBuffer body;
    char** headers;
    char** header_values;
    int header_count;
    int header_capacity;
} Response;

typedef struct {
    char* method;
    char* path;
    char* query;
    DynamicBuffer body;
    char** headers;
    char** header_values;
    int header_count;
    char* param_names[WF_MAX_PARAMS];
    char* param_values[WF_MAX_PARAMS];
    int param_count;
} Request;

typedef Response* (*RouteHandler)(Request* req);

typedef struct {
    char* method;
    char* path_pattern;
    char** param_names;
    int param_count;
    RouteHandler handler;
} Route;

extern Route routes[WF_MAX_ROUTES];
extern int route_count;

// ============================================================================
// JSON Builder - Dynamic Buffer
// ============================================================================

typedef struct {
    DynamicBuffer buffer;
    bool first_key;
    int depth;
} JsonBuilder;

void jb_init(JsonBuilder* jb);
void jb_start_object(JsonBuilder* jb);
void jb_end_object(JsonBuilder* jb);
void jb_start_array(JsonBuilder* jb);
void jb_end_array(JsonBuilder* jb);

// Object key-value pairs
void jb_add_string(JsonBuilder* jb, const char* key, const char* value);
void jb_add_int(JsonBuilder* jb, const char* key, long value);
void jb_add_float(JsonBuilder* jb, const char* key, double value);
void jb_add_bool(JsonBuilder* jb, const char* key, bool value);
void jb_add_null(JsonBuilder* jb, const char* key);
void jb_key(JsonBuilder* jb, const char* key);
void jb_add_timestamp(JsonBuilder* jb, const char* key);  // Numeric timestamp

// Array elements (no key)
void jb_arr_string(JsonBuilder* jb, const char* value);
void jb_arr_int(JsonBuilder* jb, long value);
void jb_arr_float(JsonBuilder* jb, double value);
void jb_arr_bool(JsonBuilder* jb, bool value);
void jb_arr_null(JsonBuilder* jb);
void jb_arr_start_object(JsonBuilder* jb);
void jb_arr_start_array(JsonBuilder* jb);

const char* jb_get(JsonBuilder* jb);
void jb_free(JsonBuilder* jb);

// ============================================================================
// SEO Metadata -> HTML
// ============================================================================

typedef struct {
    char* title;
    char* description;
    char* keywords;
    char* canonical;
    char* og_title;
    char* og_description;
    char* og_image;
    char* og_type;
    char* twitter_card;
} SeoMetadata;

Response* webfast_render_html_with_seo(const char* body, const SeoMetadata* seo);
void seo_free(SeoMetadata* seo);

// ============================================================================
// Template Engine
// ============================================================================

char* webfast_render_template(const char* template_path, const char* data_json);

// ============================================================================
// Route Registration & Server
// ============================================================================

void webfast_register_route(const char* method, const char* path, RouteHandler handler);
void webfast_run(int port);
void webfast_run_with_keepalive(int port, int timeout_seconds);

// Request helpers (no memory leaks)
const char* webfast_get_param(Request* req, const char* name);
const char* webfast_get_query_param(Request* req, const char* name);
const char* webfast_get_header(Request* req, const char* name);
const char* webfast_get_body(Request* req);

// Response creators
Response* webfast_json_response(const char* json);
Response* webfast_text_response(const char* text);
Response* webfast_html_response(const char* html);
Response* webfast_redirect(const char* url, int status_code);
Response* webfast_error(int status_code, const char* message);

void webfast_set_header(Response* resp, const char* key, const char* value);

// Utility
long webfast_timestamp(void);  // Returns numeric timestamp
char* webfast_uuid(void);      // Secure UUID with /dev/urandom

// Cleanup
void webfast_free_response(Response* resp);
void webfast_free_request(Request* req);

#endif
