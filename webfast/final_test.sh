#!/bin/bash
set -e
cd /home/runner/work/rayana/rayana/webfast

# Start server
./examples/comprehensive_test > /tmp/server.log 2>&1 &
SERVER_PID=$!
sleep 3

echo "Server started with PID: $SERVER_PID"
echo ""

# Test function
test_endpoint() {
    local name="$1"
    local url="$2"
    local method="${3:-GET}"
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Test: $name"
    echo "URL: $method $url"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ "$method" = "POST" ]; then
        response=$(curl -s -X POST -H "Content-Type: application/json" -d '{}' "$url" --max-time 5 2>&1)
    elif [ "$method" = "PUT" ]; then
        response=$(curl -s -X PUT -H "Content-Type: application/json" -d '{}' "$url" --max-time 5 2>&1)
    elif [ "$method" = "DELETE" ]; then
        response=$(curl -s -X DELETE "$url" --max-time 5 2>&1)
    else
        response=$(curl -s "$url" --max-time 5 2>&1)
    fi
    
    # Try to validate JSON
    if echo "$response" | python3 -m json.tool > /dev/null 2>&1; then
        echo "✅ Valid JSON"
        echo "$response" | python3 -m json.tool | head -20
    else
        echo "⚠️  Not JSON or invalid HTML"
        echo "$response" | head -10
    fi
    echo ""
}

# Run all tests
test_endpoint "Home Page" "http://localhost:8000/"
test_endpoint "Arrays Test" "http://localhost:8000/test/arrays"
test_endpoint "Nested Structures" "http://localhost:8000/test/nested"
test_endpoint "SEO to HTML" "http://localhost:8000/test/seo"
test_endpoint "Route Parameter" "http://localhost:8000/user/123"
test_endpoint "Nested Params" "http://localhost:8000/post/42/comment/7"
test_endpoint "Query String" "http://localhost:8000/search?q=webfast"
test_endpoint "All Types" "http://localhost:8000/test/types"
test_endpoint "Config Access" "http://localhost:8000/test/config"
test_endpoint "Text Response" "http://localhost:8000/robots.txt"
test_endpoint "POST Method" "http://localhost:8000/api/create" "POST"
test_endpoint "PUT Method" "http://localhost:8000/api/update/99" "PUT"
test_endpoint "DELETE Method" "http://localhost:8000/api/delete/99" "DELETE"
test_endpoint "Health Check" "http://localhost:8000/health"
test_endpoint "Dashboard" "http://localhost:8000/api/dashboard"
test_endpoint "About Page" "http://localhost:8000/about"

# Cleanup
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ All tests completed!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
