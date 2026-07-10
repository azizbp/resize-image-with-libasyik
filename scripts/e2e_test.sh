#!/usr/bin/env bash
# End-to-end test for the /resize_image service.
#
# Usage:
#   ./scripts/e2e_test.sh <input.jpg> [host:port]   (default: localhost:8080)
#
# The service must already be running, e.g.:
#   docker run --rm -d -p 8080:8080 --name resize-service resize-service
set -euo pipefail

INPUT_JPEG="${1:?usage: $0 <input.jpg> [host:port]}"
HOST="${2:-localhost:8080}"
URL="http://$HOST/resize_image"

command -v jq >/dev/null || { echo "jq is required: sudo apt-get install -y jq"; exit 1; }

echo "=== 1. happy path: resize $INPUT_JPEG to 100x50 ==="

# request payload, same shape as input_payload.json
BASE64_IMG=$(base64 -w0 "$INPUT_JPEG")
printf '{\n"input_jpeg": "%s",\n"desired_width": 100,\n"desired_height": 50\n}\n' \
    "$BASE64_IMG" > /tmp/input_payload.json

HTTP_CODE=$(curl -s -o /tmp/response.json -w "%{http_code}" -X POST "$URL" \
    -H 'Content-Type: application/json' -d @/tmp/input_payload.json)

echo "HTTP status: $HTTP_CODE"
jq 'del(.output_jpeg)' /tmp/response.json  # print payload minus the long base64
test "$HTTP_CODE" = "200"
test "$(jq -r .message /tmp/response.json)" = "success"

# decode output_jpeg and prove it is a real 100x50 JPEG
jq -r .output_jpeg /tmp/response.json | base64 -d > /tmp/output.jpg
file /tmp/output.jpg | tee /dev/stderr | grep -q "JPEG image data"
file /tmp/output.jpg | grep -q "100x50"
echo "OK: output_jpeg decodes to a 100x50 JPEG (saved at /tmp/output.jpg)"

echo
echo "=== 2. error cases: must return 4xx with capital-M \"Message\" ==="

check_error() {
    local name="$1" payload="$2" expected="$3" code
    printf '%s' "$payload" > /tmp/error_payload.json
    code=$(curl -s -o /tmp/response.json -w "%{http_code}" -X POST "$URL" \
        -H 'Content-Type: application/json' -d @/tmp/error_payload.json)
    echo "-- $name -> HTTP $code: $(cat /tmp/response.json)"
    test "$code" = "$expected"
    jq -e 'has("Message") and (has("message") | not)' /tmp/response.json >/dev/null
}

check_error "malformed JSON"    '{bukan json'                                                          400
check_error "missing fields"    '{"desired_width": 100}'                                               400
check_error "invalid base64"    '{"input_jpeg":"!!!","desired_width":100,"desired_height":50}'         400
check_error "not a JPEG"        '{"input_jpeg":"AAAABBBB","desired_width":100,"desired_height":50}'    400
check_error "zero width"        "{\"input_jpeg\":\"$BASE64_IMG\",\"desired_width\":0,\"desired_height\":50}"  400
check_error "negative height"   "{\"input_jpeg\":\"$BASE64_IMG\",\"desired_width\":100,\"desired_height\":-3}" 400
check_error "width too large"   "{\"input_jpeg\":\"$BASE64_IMG\",\"desired_width\":100000,\"desired_height\":50}" 400

echo
echo "ALL TESTS PASSED"
