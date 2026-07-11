#pragma once
#include <string>
#include <vector>
#include "SecureString.h"
#include <QString>
#include "json.hpp" 


struct Note {
    long long Id = 0;
    std::string Name;                 
    std::string SecuredText;          
    std::string PreviousHash;         
    std::vector<std::string> Tags;   
};

std::string CalculateSHA256(const std::string& input);

std::string EncryptAES(const SecureString& plainText, const SecureString& password, const std::vector<unsigned char>& salt);

SecureString DecryptAES(const std::string& encryptedData, const SecureString& password, const std::vector<unsigned char>& salt);

SecureString GetRandonDEK();

std::vector<unsigned char> DerivedKey(const SecureString& text, const std::vector<unsigned char>& salt);

std::string GenerateRandomSalt();

SecureString QStringToSecureString(const QString& qtStr);

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