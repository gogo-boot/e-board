#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "sec/aes_crypto.h"

// Platform-specific includes
#ifdef ARDUINO
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <esp_system.h>
#else
#include <openssl/aes.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <random>
#endif

// Constructor with user key
AESCrypto::AESCrypto(const std::string& userKey) : m_key(AES_KEY_SIZE, 0), m_keySet(false) {
    setKey(userKey);
}

// Default constructor
AESCrypto::AESCrypto() : m_key(AES_KEY_SIZE, 0), m_keySet(false) {}

// Set encryption key
void AESCrypto::setKey(const std::string& userKey) {
    generateKeyFromString(userKey);
    m_keySet = true;
}

// Generate key from user string using MD5
void AESCrypto::generateKeyFromString(const std::string& userKey) {
#ifdef ARDUINO
    // ESP32/Arduino implementation using mbedTLS
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);

    int ret = mbedtls_md_setup(&md_ctx, md_info, 0);
    if (ret == 0) {
        mbedtls_md_starts(&md_ctx);
        mbedtls_md_update(&md_ctx, (const unsigned char*)userKey.c_str(), userKey.length());
        mbedtls_md_finish(&md_ctx, m_key.data());
    }
    mbedtls_md_free(&md_ctx);
#else
    // Standard C++ implementation using OpenSSL
    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, userKey.c_str(), userKey.length());
    MD5_Final(m_key.data(), &md5_ctx);
#endif
}

// Generate random IV
void AESCrypto::generateRandomIV(uint8_t* iv) const {
#ifdef ARDUINO
    // ESP32 hardware random number generator
    esp_fill_random(iv, IV_SIZE);
#else
    // Standard C++ random generation
    if (RAND_bytes(iv, IV_SIZE) != 1) {
        // Fallback to C++ random if OpenSSL fails
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < IV_SIZE; i++) {
            iv[i] = static_cast<uint8_t>(dis(gen));
        }
    }
#endif
}

// Apply PKCS7 padding
size_t AESCrypto::applyPKCS7Padding(const uint8_t* input, size_t inputLen, uint8_t* output) const {
    size_t paddingBytes = AES_BLOCK_SIZE - (inputLen % AES_BLOCK_SIZE);
    size_t paddedLen = inputLen + paddingBytes;

    memcpy(output, input, inputLen);
    for (size_t i = inputLen; i < paddedLen; i++) {
        output[i] = static_cast<uint8_t>(paddingBytes);
    }
    return paddedLen;
}

// Remove PKCS7 padding
size_t AESCrypto::removePKCS7Padding(const uint8_t* input, size_t inputLen) const {
    if (inputLen == 0) return 0;

    uint8_t paddingBytes = input[inputLen - 1];
    if (paddingBytes > AES_BLOCK_SIZE || paddingBytes == 0) {
        return 0; // Invalid padding
    }

    // Verify padding
    for (size_t i = inputLen - paddingBytes; i < inputLen; i++) {
        if (input[i] != paddingBytes) {
            return 0; // Corrupted padding
        }
    }
    return inputLen - paddingBytes;
}

// Encrypt plaintext
AESCrypto::EncryptedData AESCrypto::encrypt(const std::string& plaintext) const {
    EncryptedData result;

    if (!m_keySet) {
        return result; // Return empty result if key not set
    }

    // Generate random IV
    generateRandomIV(result.iv.data());

    // Prepare input data
    const uint8_t* input = reinterpret_cast<const uint8_t*>(plaintext.c_str());
    size_t inputLen = plaintext.length();

    // Calculate padded length and apply padding
    size_t maxPaddedLen = inputLen + AES_BLOCK_SIZE;
    std::vector<uint8_t> paddedData(maxPaddedLen);
    size_t paddedLen = applyPKCS7Padding(input, inputLen, paddedData.data());

    // Prepare output buffer
    result.encrypted_data.resize(paddedLen);

#ifdef ARDUINO
    // ESP32/Arduino implementation using mbedTLS
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);

    int ret = mbedtls_aes_setkey_enc(&aes_ctx, m_key.data(), AES_KEY_SIZE * 8);
    if (ret == 0) {
        std::vector<uint8_t> ivCopy = result.iv; // mbedtls modifies IV
        ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, paddedLen,
                                    ivCopy.data(), paddedData.data(), result.encrypted_data.data());
    }
    mbedtls_aes_free(&aes_ctx);

    if (ret != 0) {
        result.encrypted_data.clear();
    }
#else
    // Standard C++ implementation using OpenSSL
    AES_KEY aes_key;
    if (AES_set_encrypt_key(m_key.data(), AES_KEY_SIZE * 8, &aes_key) == 0) {
        std::vector<uint8_t> ivCopy = result.iv; // AES_cbc_encrypt modifies IV
        AES_cbc_encrypt(paddedData.data(), result.encrypted_data.data(), paddedLen,
                        &aes_key, ivCopy.data(), AES_ENCRYPT);
    } else {
        result.encrypted_data.clear();
    }
#endif

    return result;
}

// Decrypt encrypted data
std::string AESCrypto::decrypt(const EncryptedData& encryptedData) const {
    if (!m_keySet || encryptedData.encrypted_data.empty()) {
        return "";
    }

    std::vector<uint8_t> decryptedData(encryptedData.encrypted_data.size());

#ifdef ARDUINO
    // ESP32/Arduino implementation using mbedTLS
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);

    int ret = mbedtls_aes_setkey_dec(&aes_ctx, m_key.data(), AES_KEY_SIZE * 8);
    if (ret == 0) {
        std::vector<uint8_t> ivCopy = encryptedData.iv; // mbedtls modifies IV
        ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, encryptedData.encrypted_data.size(),
                                    ivCopy.data(), encryptedData.encrypted_data.data(), decryptedData.data());
    }
    mbedtls_aes_free(&aes_ctx);

    if (ret != 0) {
        return "";
    }
#else
    // Standard C++ implementation using OpenSSL
    AES_KEY aes_key;
    if (AES_set_decrypt_key(m_key.data(), AES_KEY_SIZE * 8, &aes_key) != 0) {
        return "";
    }

    std::vector<uint8_t> ivCopy = encryptedData.iv; // AES_cbc_encrypt modifies IV
    AES_cbc_encrypt(encryptedData.encrypted_data.data(), decryptedData.data(),
                    encryptedData.encrypted_data.size(), &aes_key, ivCopy.data(), AES_DECRYPT);
#endif

    // Remove padding
    size_t actualLen = removePKCS7Padding(decryptedData.data(), decryptedData.size());
    if (actualLen == 0) {
        return "";
    }

    return std::string(reinterpret_cast<const char*>(decryptedData.data()), actualLen);
}

// Convenience methods
std::string AESCrypto::encryptToHex(const std::string& plaintext) const {
    EncryptedData encrypted = encrypt(plaintext);
    return encrypted.toHexString();
}

std::string AESCrypto::decryptFromHex(const std::string& hexString) const {
    EncryptedData encrypted = EncryptedData::fromHexString(hexString);
    return decrypt(encrypted);
}

// Utility methods
std::string AESCrypto::bytesToHex(const uint8_t* data, size_t length) const {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < length; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::vector<uint8_t> AESCrypto::hexToBytes(const std::string& hexStr) const {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        std::string byteStr = hexStr.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// EncryptedData methods
std::string AESCrypto::EncryptedData::toHexString() const {
    AESCrypto crypto; // Temporary instance for utility methods
    std::string result = crypto.bytesToHex(iv.data(), iv.size());
    result += crypto.bytesToHex(encrypted_data.data(), encrypted_data.size());
    return result;
}

AESCrypto::EncryptedData AESCrypto::EncryptedData::fromHexString(const std::string& hexStr) {
    EncryptedData result;
    if (hexStr.length() < IV_SIZE * 2) {
        return result; // Invalid hex string
    }

    AESCrypto crypto; // Temporary instance for utility methods
    std::vector<uint8_t> allBytes = crypto.hexToBytes(hexStr);

    if (allBytes.size() < IV_SIZE) {
        return result; // Invalid data
    }

    // Extract IV
    result.iv.assign(allBytes.begin(), allBytes.begin() + IV_SIZE);

    // Extract encrypted data
    result.encrypted_data.assign(allBytes.begin() + IV_SIZE, allBytes.end());

    return result;
}

#include "secrets/general_secrets.h"
// Static utility methods for API key decryption (NO CACHING for security)
std::string AESCrypto::getRMVAPIKey() {
    // Decrypt fresh each time - no caching for security
    AESCrypto crypto(ENCRYPTION_KEY);

    // Include the encrypted RMV API key

    // Decrypt and return immediately (no storage)
    std::string decryptedKey = crypto.decryptFromHex(RMV_API_KEY);

    // Clear the crypto object to minimize memory exposure
    crypto = AESCrypto(); // Reset to clear internal key

    return decryptedKey;
}

std::string AESCrypto::getGoogleAPIKey() {
    // Decrypt fresh each time - no caching for security
    AESCrypto crypto(ENCRYPTION_KEY);

    // Include the encrypted Google API key

    // Decrypt and return immediately (no storage)
    std::string decryptedKey = crypto.decryptFromHex(GOOGLE_API_KEY);
    // Clear the crypto object to minimize memory exposure
    crypto = AESCrypto(); // Reset to clear internal key

    return decryptedKey;
}
