#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// Function to calculate HMAC SHA256 and return raw binary result
std::string calculate_hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    HMAC(EVP_sha256(), key.c_str(), key.length(),
         (unsigned char*)data.c_str(), data.length(),
         hash, &hash_len);
    
    // Return the raw binary HMAC result
    return std::string(reinterpret_cast<char*>(hash), hash_len);
}

// Function to convert binary HMAC result to hex string
std::string hmac_to_hex(const std::string& hmac) {
    std::stringstream ss;
    for (unsigned char c : hmac) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return ss.str();
}

// Function to calculate SHA256 hash and return as hex string
std::string sha256_hex(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Function to get the current time in a specific format
std::string get_current_time(const char* format) {
    time_t now = time(0);
    char buf[64];
    strftime(buf, sizeof(buf), format, gmtime(&now));
    return std::string(buf);
}

// Function to upload a file to S3
void upload_to_s3(const std::string& access_key, const std::string& secret_key,
                  const std::string& region, const std::string& bucket,
                  const std::string& file_path, const std::string& object_key) {
    // Get current timestamp
    std::string amz_date = get_current_time("%Y%m%dT%H%M%SZ");
    std::string date_stamp = get_current_time("%Y%m%d");

    // Host and endpoint
    std::string host = bucket + ".s3." + region + ".amazonaws.com";
    std::string endpoint = "https://" + host + "/" + object_key;

    // Read file and calculate hash
    std::ifstream file(file_path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string file_content = buffer.str();
    std::string payload_hash = sha256_hex(file_content);

    // Create canonical request
    std::string canonical_uri = "/" + object_key;
    std::string canonical_headers = "host:" + host + "\n" +
                                     "x-amz-content-sha256:" + payload_hash + "\n" +
                                     "x-amz-date:" + amz_date + "\n";
    std::string signed_headers = "host;x-amz-content-sha256;x-amz-date";
    std::string canonical_request = "PUT\n" + canonical_uri + "\n\n" +
                                    canonical_headers + "\n" +
                                    signed_headers + "\n" + payload_hash;

    // Create string to sign
    std::string algorithm = "AWS4-HMAC-SHA256";
    std::string credential_scope = date_stamp + "/" + region + "/s3/aws4_request";
    std::string string_to_sign = algorithm + "\n" + amz_date + "\n" +
                                  credential_scope + "\n" +
                                  sha256_hex(canonical_request);

    // Calculate signature
    std::string k_date = calculate_hmac_sha256("AWS4" + secret_key, date_stamp);
    std::string k_region = calculate_hmac_sha256(k_date, region);
    std::string k_service = calculate_hmac_sha256(k_region, "s3");
    std::string k_signing = calculate_hmac_sha256(k_service, "aws4_request");
    std::string signature = hmac_to_hex(calculate_hmac_sha256(k_signing, string_to_sign)); // Convert to hex

    // Create authorization header
    std::string authorization = algorithm + " Credential=" + access_key + "/" + credential_scope +
                                ", SignedHeaders=" + signed_headers + ", Signature=" + signature;

    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: " + authorization).c_str());
        headers = curl_slist_append(headers, ("x-amz-date: " + amz_date).c_str());
        headers = curl_slist_append(headers, ("x-amz-content-sha256: " + payload_hash).c_str());
        headers = curl_slist_append(headers, "Content-Type: image/jpeg"); // Change to the appropriate content type

        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        FILE* fp1 = fopen(file_path.c_str(), "rb");
        if (!fp1) 
          return;
        curl_easy_setopt(curl, CURLOPT_READDATA, fp1);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_content.size());

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Upload failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
          std::cout << "Upload success" << std::endl;
        }

        fclose(fp1);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

int main() {
    const std::string ACCESS_KEY = "xxxxxxx_KEY";
    const std::string SECRET_KEY = "xxxxxxx_SECRET";
    const std::string REGION = "us-east-2"; // Change to your region
    const std::string BUCKET = "xxxxxxx_BUCKET";
    const std::string FILE_PATH = "./test.txt"; // Path to your image
    const std::string OBJECT_KEY = "xxxxxxx_OBJECT_KEY"; // S3 object key


    upload_to_s3(ACCESS_KEY, SECRET_KEY, REGION, BUCKET, FILE_PATH, OBJECT_KEY);
    return 0;
}