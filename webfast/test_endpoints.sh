#!/bin/bash
cd webfast

# Start server in background
./examples/comprehensive_test &
SERVER_PID=$!
sleep 1

echo "=== Testing endpoints ==="
echo ""

echo "1. GET / (home)"
curl -s http://localhost:8080/ | head -20
echo ""
echo ""

echo "2. GET /health"
curl -s http://localhost:8080/health
echo ""
echo ""

echo "3. GET /about"
curl -s http://localhost:8080/about
echo ""
echo ""

echo "4. GET /user/42 (route param)"
curl -s http://localhost:8080/user/42
echo ""
echo ""

echo "5. GET /post/10/comment/5 (nested params)"
curl -s http://localhost:8080/post/10/comment/5
echo ""
echo ""

echo "6. GET /search?q=test (query string)"
curl -s "http://localhost:8080/search?q=test"
echo ""
echo ""

echo "7. GET /test/if (if statement)"
curl -s http://localhost:8080/test/if
echo ""
echo ""

echo "8. GET /test/vars (variables)"
curl -s http://localhost:8080/test/vars
echo ""
echo ""

echo "9. GET /test/types (data types)"
curl -s http://localhost:8080/test/types
echo ""
echo ""

echo "10. GET /robots.txt (text response)"
curl -s http://localhost:8080/robots.txt
echo ""
echo ""

echo "11. GET /api/version"
curl -s http://localhost:8080/api/version
echo ""
echo ""

echo "12. GET /api/complex (nested objects)"
curl -s http://localhost:8080/api/complex
echo ""
echo ""

# Kill server
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "=== Test complete ==="
