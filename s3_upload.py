import hashlib
import hmac
import datetime
import requests
import os

class S3Uploader:
    def __init__(self, access_key, secret_key, region="us-east-2"):
        self.access_key = access_key
        self.secret_key = secret_key
        self.region = region

    def sign(self, key, msg):
        return hmac.new(key, msg.encode('utf-8'), hashlib.sha256).digest()

    def get_signature_key(self, date_stamp, region_name, service_name):
        k_date = self.sign(('AWS4' + self.secret_key).encode('utf-8'), date_stamp)
        k_region = self.sign(k_date, region_name)
        k_service = self.sign(k_region, service_name)
        k_signing = self.sign(k_service, 'aws4_request')
        return k_signing

    def upload_file(self, file_path, bucket, object_key):
        # Request values
        t = datetime.datetime.utcnow()
        amz_date = t.strftime('%Y%m%dT%H%M%SZ')
        date_stamp = t.strftime('%Y%m%d')

        print(amz_date)
        print(date_stamp)
        
        # Host and endpoints
        host = f"{bucket}.s3.{self.region}.amazonaws.com"
        endpoint = f"https://{host}/{object_key}"

        # Read file and get hash
        with open(file_path, 'rb') as f:
            file_content = f.read()
            payload_hash = hashlib.sha256(file_content).hexdigest()

        # Canonical Request
        canonical_uri = '/' + object_key
        canonical_querystring = ''
        canonical_headers = (
            f'host:{host}\n'
            f'x-amz-content-sha256:{payload_hash}\n'
            f'x-amz-date:{amz_date}\n'
        )
        signed_headers = 'host;x-amz-content-sha256;x-amz-date'
        canonical_request = '\n'.join([
            'PUT',
            canonical_uri,
            canonical_querystring,
            canonical_headers,
            signed_headers,
            payload_hash
        ])

        # String to Sign
        algorithm = 'AWS4-HMAC-SHA256'
        credential_scope = f'{date_stamp}/{self.region}/s3/aws4_request'
        string_to_sign = '\n'.join([
            algorithm,
            amz_date,
            credential_scope,
            hashlib.sha256(canonical_request.encode('utf-8')).hexdigest()
        ])

        # Calculate signature
        signing_key = self.get_signature_key(date_stamp, self.region, 's3')
        signature = hmac.new(signing_key, string_to_sign.encode('utf-8'), hashlib.sha256).hexdigest()

        # Create authorization header
        authorization_header = (
            f'{algorithm} '
            f'Credential={self.access_key}/{credential_scope}, '
            f'SignedHeaders={signed_headers}, '
            f'Signature={signature}'
        )

        # Headers for request
        headers = {
            'Authorization': authorization_header,
            'x-amz-date': amz_date,
            'x-amz-content-sha256': payload_hash,
            'Content-Type': 'application/octet-stream'
        }

        # Print debug information
        print("Canonical Request:")
        print(canonical_request)
        print("\nString to Sign:")
        print(string_to_sign)
        print("\nAuthorization:")
        print(authorization_header)

        # Send request
        response = requests.put(endpoint, data=file_content, headers=headers)
        return response

# Usage example
if __name__ == "__main__":
    ACCESS_KEY = "xxxxxxx_KEY"
    SECRET_KEY = "xxxxxxx_SECRET"
    BUCKET = "xxxxxxx_BUCKET"
    
    uploader = S3Uploader(ACCESS_KEY, SECRET_KEY)
    
    # Upload file
    response = uploader.upload_file(
        file_path="./test.txt",
        bucket=BUCKET,
        object_key="test.txt"
    )
    
    print(f"Status Code: {response.status_code}")
    print(f"Response: {response.text}") 