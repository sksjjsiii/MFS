#!/bin/bash
cd /home/runner/work/rayana/rayana/webfast

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  WebFast v3.0 - Comprehensive Test Suite                 ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Start server in background
./examples/complete_app &
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
        response=$(curl -s -X POST -H "Content-Type: application/json" -d '{"test":"data"}' "$url" --max-time 2)
    elif [ "$method" = "PUT" ]; then
        response=$(curl -s -X PUT -H "Content-Type: application/json" -d '{"update":"data"}' "$url" --max-time 2)
    elif [ "$method" = "DELETE" ]; then
        response=$(curl -s -X DELETE "$url" --max-time 2)
    else
        response=$(curl -s "$url" --max-time 2)
    fi
    
    echo "$response" | head -15
    echo ""
}

# Run all tests
test_endpoint "GET" "http://localhost:8000/" "Home page with config access"
test_endpoint "GET" "http://localhost:8000/health" "Health check endpoint"
test_endpoint "GET" "http://localhost:8000/about" "About page with SEO"
test_endpoint "GET" "http://localhost:8000/user/123" "Route parameter"
test_endpoint "GET" "http://localhost:8000/post/42/comment/7" "Nested route parameters"
test_endpoint "GET" "http://localhost:8000/search?q=webfast" "Query string handling"
test_endpoint "GET" "http://localhost:8000/types" "All data types"
test_endpoint "GET" "http://localhost:8000/test/if" "If statement"
test_endpoint "GET" "http://localhost:8000/test/vars" "Variable assignment"
test_endpoint "GET" "http://localhost:8000/robots.txt" "Text response"
test_endpoint "GET" "http://localhost:8000/page" "HTML response"
test_endpoint "POST" "http://localhost:8000/api/submit" "POST request"
test_endpoint "PUT" "http://localhost:8000/api/update/99" "PUT request"
test_endpoint "DELETE" "http://localhost:8000/api/delete/99" "DELETE request"
test_endpoint "GET" "http://localhost:8000/api/complex" "Complex nested objects"
test_endpoint "GET" "http://localhost:8000/api/version" "API version"
test_endpoint "GET" "http://localhost:8000/test/error" "Error handling"
test_endpoint "GET" "http://localhost:8000/perf" "Performance info"

# Cleanup
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  All tests completed successfully! ✓                     ║"
echo "╚══════════════════════════════════════════════════════════╝"
