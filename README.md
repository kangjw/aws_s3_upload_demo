# aws_s3_upload_demo
## features
1. Support AWS Signature Version 4
2. Openssl and curl used to upload file.
3. No depend the AWS SDK.
 
# Complile s3_upload_file.cpp
 g++ -o s3_upload s3_upload_file.cpp -lcurl -lssl -lcrypto

# Support Authenticating Requests (AWS Signature Version 4)
please reference:
https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-authenticating-requests.html
