#!/bin/bash

# Configuration
ACCESS_KEY="xxxxxxx_KEY"
SECRET_KEY="xxxxxxx_KEY"
REGION="us-east-2"
BUCKET="xxxxxxx_BUCKET"
FILE_PATH="$1"
OBJECT_KEY="$2"

# Get current timestamp
TIMESTAMP=$(date -u +%Y%m%dT%H%M%SZ)
DATE=$(date -u +%Y%m%d)
HOST="${BUCKET}.s3.${REGION}.amazonaws.com"

# Calculate content hash
CONTENT_SHA256=$(openssl dgst -sha256 -binary < "${FILE_PATH}" | xxd -p -c 256)

# Create canonical request
CANONICAL_URI="/${OBJECT_KEY}"
CANONICAL_QUERYSTRING=""

CANONICAL_HEADERS="host:${HOST}
x-amz-content-sha256:${CONTENT_SHA256}
x-amz-date:${TIMESTAMP}"

SIGNED_HEADERS="host;x-amz-content-sha256;x-amz-date"

CANONICAL_REQUEST="PUT
${CANONICAL_URI}
${CANONICAL_QUERYSTRING}
${CANONICAL_HEADERS}

${SIGNED_HEADERS}
${CONTENT_SHA256}"

echo "Canonical Request: ${CANONICAL_REQUEST}"
# Create string to sign
CREDENTIAL_SCOPE="${DATE}/${REGION}/s3/aws4_request"
STRING_TO_SIGN="AWS4-HMAC-SHA256
${TIMESTAMP}
${CREDENTIAL_SCOPE}
$(echo -n "${CANONICAL_REQUEST}" | openssl dgst -sha256 -hex | sed 's/^.* //')"
echo "String to Sign: ${STRING_TO_SIGN}"
# Calculate signature
k_date=$(echo -n "${DATE}" | openssl dgst -sha256 -mac HMAC -macopt "key:AWS4${SECRET_KEY}" -binary)
k_region=$(echo -n "${REGION}" | openssl dgst -sha256 -mac HMAC -macopt "key:${k_date}" -binary)
k_service=$(echo -n "s3" | openssl dgst -sha256 -mac HMAC -macopt "key:${k_region}" -binary)
k_signing=$(echo -n "aws4_request" | openssl dgst -sha256 -mac HMAC -macopt "key:${k_service}" -binary)
SIGNATURE=$(echo -n "${STRING_TO_SIGN}" | openssl dgst -sha256 -mac HMAC -macopt "key:${k_signing}" -hex | sed 's/^.* //')
echo "Signature: ${SIGNATURE}"
# Create authorization header
AUTHORIZATION="AWS4-HMAC-SHA256 Credential=${ACCESS_KEY}/${CREDENTIAL_SCOPE},SignedHeaders=${SIGNED_HEADERS},Signature=${SIGNATURE}"
echo "Authorization: ${AUTHORIZATION}"

# Upload file using curl
curl -X PUT -T "${FILE_PATH}" \
  -H "Authorization: ${AUTHORIZATION}" \
  -H "x-amz-content-sha256: ${CONTENT_SHA256}" \
  -H "x-amz-date: ${TIMESTAMP}" \
  "https://${HOST}${CANONICAL_URI}" 