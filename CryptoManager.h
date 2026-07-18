#pragma once
#include <string>
#include <vector>
#include "SecureString.h"
#include <QString>
#include "json.hpp" 

struct AttachmentInfo {
    std::string OriginalNameEncrypted;    // Чтобы знать, как файл назывался
    std::string DiskFileName;             // Чтобы найти его в папке attachments
    std::vector<unsigned char> Iv;        // Чтобы расшифровать
    long long FileSize = 0;
    std::string FileHash;
};

struct Note {
    long long Id = 0;
    std::string Name;
    std::string SecuredText;          
    std::string PreviousHash;         
    std::vector<std::string> Tags;   
    std::vector<AttachmentInfo> Attachments;
};

std::string CalculateFileSHA256(const QString& filePath);

SecureString QStringToSecureString(const QString& qtStr);

std::string EncodeBase32(const unsigned char* data, size_t length);

std::string Generate2FASecret(SecureString& outRawSecret);

int GenerateTOTP(const SecureString& secret, long long timeWindow);

bool Verify2FACode(const SecureString& secret, int userCode);

std::string CalculateSHA256(const std::string& input);

std::string EncryptAES(const SecureString& plainText, const SecureString& password, const std::vector<unsigned char>& salt);

SecureString DecryptAES(const std::string& encryptedData, const SecureString& password, const std::vector<unsigned char>& salt);

SecureString GetRandonDEK();

std::vector<unsigned char> DerivedKey(const SecureString& text, const std::vector<unsigned char>& salt);

std::string GenerateRandomSalt();

std::string StringToHex(const std::string input);

SecureString SecureStringToHex(const SecureString input);

std::string HexToString(const std::string& input);

SecureString HexToSecureString(const SecureString& input);

std::string MerkleRoot(const std::vector<Note>& UserNotes);

long long PoliHash(std::string word, long long p, long long q);

inline void to_json(nlohmann::json& j, const Note& n) {
    j = nlohmann::json{
        {"name", n.Name},
        {"secured_text", n.SecuredText},
        {"tags", n.Tags}
    };
}

// Функция автоматической конвертации ИЗ JSON-объекта В структуру
inline void from_json(const nlohmann::json& j, Note& n) {
    j.at("name").get_to(n.Name);
    j.at("secured_text").get_to(n.SecuredText);
    j.at("tags").get_to(n.Tags);
}

unsigned int EncryptFileStream(
    const std::string& inputPath,
    const std::string& outputPath,
    const SecureString& key,
    const std::vector<unsigned char>& salt,
    const std::vector<unsigned char>& iv
);

bool DecryptFileStream(
    const std::string& inputPath,
    const std::string& outputPath,
    const SecureString& key,
    const std::vector<unsigned char>& salt,
    const std::vector<unsigned char>& iv
);