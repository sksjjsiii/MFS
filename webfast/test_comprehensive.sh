#!/bin/bash
set -e

cd /home/runner/work/rayana/rayana/webfast

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  WebFast v4.0 - Comprehensive Test Suite                 ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Start server in background
echo "Starting server..."
./examples/comprehensive_test &
SERVER_PID=$!
sleep 2

echo "Server started with PID: $SERVER_PID"
echo ""

# Test function
test_endpoint() {
    local method=$1
    local url=$2
    local description=$3
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Test: $method $url"
    echo "Description: $description"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ "$method" = "POST" ]; then
        response=$(curl -s -X POST -H "Content-Type: application/json" -d '{}' "$url" --max-time 2 2>&1)
    elif [ "$method" = "PUT" ]; then
        response=$(curl -s -X PUT -H "Content-Type: application/json" -d '{}' "$url" --max-time 2 2>&1)
    elif [ "$method" = "DELETE" ]; then
        response=$(curl -s -X DELETE "$url" --max-time 2 2>&1)
    else
        response=$(curl -s "$url" --max-time 2 2>&1)
    fi
    
    echo "$response" | head -20
    echo ""
}

# Run all tests
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 1: Home Page with SEO                              ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/" "Home page with features list"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 2: Arrays in JSON (CRITICAL)                       ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/test/arrays" "Arrays should render as [1,2,3] not null"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 3: Nested Structures                               ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/test/nested" "Nested arrays and objects"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 4: SEO to HTML (CRITICAL)                          ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/test/seo" "SEO meta tags in HTML"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 5: Route Parameters                                ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/user/123" "Single parameter"
test_endpoint "GET" "http://localhost:8000/post/42/comment/7" "Multiple parameters"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 6: Query Strings                                   ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/search?q=webfast" "Query string handling"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 7: All Data Types                                  ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/test/types" "All data types"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 8: Config Access                                   ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/test/config" "Config values"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 9: Text Response                                   ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/robots.txt" "Plain text response"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 10: POST Method                                    ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "POST" "http://localhost:8000/api/create" "POST request"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 11: PUT Method                                     ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "PUT" "http://localhost:8000/api/update/99" "PUT request"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 12: DELETE Method                                  ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "DELETE" "http://localhost:8000/api/delete/99" "DELETE request"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 13: Health Check                                   ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/health" "Health endpoint"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 14: Complex Dashboard                              ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/api/dashboard" "Complex nested data"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  TEST 15: About Page with SEO                            ║"
echo "╚══════════════════════════════════════════════════════════╝"
test_endpoint "GET" "http://localhost:8000/about" "About page with SEO"

# Cleanup
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  All tests completed!                                     ║"
echo "╚══════════════════════════════════════════════════════════╝"
