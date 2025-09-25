#pragma once

#include <string>
#include <vector>

/**
 * Cross-platform AES-128-CBC encryption/decryption library
 * Works with both standard C++ (using OpenSSL) and PlatformIO/ESP32 (using mbedTLS)
 */
class AESCrypto {
public:
    // AES configuration constants
    static const size_t AES_BLOCK_SIZE = 16; // AES block size in bytes
    static const size_t AES_KEY_SIZE = 16; // AES-128 key size in bytes
    static const size_t IV_SIZE = 16; // Initialization Vector size

    /**
     * Structure to hold encrypted data with IV
     */
    struct EncryptedData {
        std::vector<uint8_t> iv; // Initialization Vector
        std::vector<uint8_t> encrypted_data; // Encrypted payload

        EncryptedData() : iv(IV_SIZE, 0) {}

        // Convert to hex string for storage/transmission
        std::string toHexString() const;

        // Create from hex string
        static EncryptedData fromHexString(const std::string& hexStr);
    };

    /**
     * Constructor with user-defined key string
     * @param userKey User-defined encryption key string
     */
    explicit AESCrypto(const std::string& userKey);

    /**
     * Default constructor - key must be set later
     */
    AESCrypto();

    /**
     * Set the encryption key
     * @param userKey User-defined encryption key string
     */
    void setKey(const std::string& userKey);

    /**
     * Encrypt plaintext string using AES-128-CBC
     * @param plaintext String to encrypt
     * @return EncryptedData structure containing IV and encrypted data
     */
    EncryptedData encrypt(const std::string& plaintext) const;

    /**
     * Decrypt encrypted data using AES-128-CBC
     * @param encryptedData EncryptedData structure from encrypt()
     * @return Decrypted plaintext string
     */
    std::string decrypt(const EncryptedData& encryptedData) const;

    /**
     * Convenience method: encrypt and return as hex string
     * @param plaintext String to encrypt
     * @return Hex string representation of encrypted data
     */
    std::string encryptToHex(const std::string& plaintext) const;

    /**
     * Convenience method: decrypt from hex string
     * @param hexString Hex string from encryptToHex()
     * @return Decrypted plaintext string
     */
    std::string decryptFromHex(const std::string& hexString) const;

    /**
     * Convert byte array to hex string (utility method)
     * @param data Byte array
     * @param length Array length
     * @return Hex string
     */
    std::string bytesToHex(const uint8_t* data, size_t length) const;

    // Static utility methods for API key decryption (NO CACHING for security)
    static std::string getRMVAPIKey();
    static std::string getGoogleAPIKey();

private:
    std::vector<uint8_t> m_key; // AES encryption key (128-bit)
    bool m_keySet; // Flag to check if key is set

    /**
     * Generate AES key from user string using MD5 hash
     * @param userKey User-defined key string
     */
    void generateKeyFromString(const std::string& userKey);

    /**
     * Apply PKCS7 padding to input data
     * @param input Input data
     * @param inputLen Input data length
     * @param output Output buffer (must be large enough)
     * @return Padded data length
     */
    size_t applyPKCS7Padding(const uint8_t* input, size_t inputLen, uint8_t* output) const;

    /**
     * Remove PKCS7 padding from decrypted data
     * @param input Input data with padding
     * @param inputLen Input data length
     * @return Actual data length after removing padding (0 if invalid)
     */
    size_t removePKCS7Padding(const uint8_t* input, size_t inputLen) const;

    /**
     * Generate random Initialization Vector
     * @param iv Buffer to store IV (must be IV_SIZE bytes)
     */
    void generateRandomIV(uint8_t* iv) const;

    /**
     * Convert hex string to byte array
     * @param hexStr Hex string
     * @return Byte vector
     */
    std::vector<uint8_t> hexToBytes(const std::string& hexStr) const;
};
