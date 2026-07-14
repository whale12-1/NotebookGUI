#include "CryptoManager.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include <QDateTime>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <string>
#include <stdexcept>
std::string EncodeBase32(const unsigned char* data, size_t length) {
	const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
	std::string result;
	int buffer = 0;
	int bitsLeft = 0;

	for (size_t i = 0; i < length; ++i) {
		buffer = (buffer << 8) | data[i];
		bitsLeft += 8;
		while (bitsLeft >= 5) {
			bitsLeft -= 5;
			result.push_back(alphabet[(buffer >> bitsLeft) & 0x1F]);
		}
	}
	if (bitsLeft > 0) {
		buffer <<= (5 - bitsLeft);
		result.push_back(alphabet[buffer & 0x1F]);
	}

	// Добавляем выравнивание '=' до кратности 8 символов (стандарт RFC)
	while (result.length() % 8 != 0) {
		result.push_back('=');
	}
	return result;
}
std::string Generate2FASecret(SecureString& outRawSecret) {
	const size_t secretLengthBytes = 10; // 10 сырых байт дают 16 символов Base32
	unsigned char buffer[secretLengthBytes];

	if (RAND_bytes(buffer, secretLengthBytes) != 1) {
		throw std::runtime_error("Ошибка генерации случайных чисел OpenSSL");
	}

	// Сохраняем строго бинарные данные
	outRawSecret = SecureString(reinterpret_cast<const char*>(buffer), secretLengthBytes);

	// Кодируем в текст для пользователя
	std::string base32String = EncodeBase32(buffer, secretLengthBytes);

	OPENSSL_cleanse(buffer, secretLengthBytes);
	return base32String;
}

long long PoliHash(std::string word, long long p, long long q) {
	long long res = 1;
	for (auto& c : word) {
		res = (res * p + c) % q;
	}
	return res;
}

std::string StringToHex(const std::string input) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (unsigned char c : input) {
		ss << std::setw(2) << (int)c;
	}
	return ss.str();
}
int GenerateTOTP(const SecureString& secret, long long timeWindow) {
	unsigned char hmacResult[20];
	unsigned int hmacLength = 0;

	// Переводим временное окно в байты (Big Endian)
	unsigned char challenge[8];
	for (int i = 7; i >= 0; i--) {
		challenge[i] = timeWindow & 0xFF;
		timeWindow >>= 8;
	}

	// Считаем HMAC-SHA1 от СЫРЫХ байт секрета
	HMAC(EVP_sha1(), secret.data(), secret.length(), challenge, 8, hmacResult, &hmacLength);

	// Динамическое усечение (Dynamic Truncation) RFC 4226
	int offset = hmacResult[19] & 0xF;
	int binCode = ((hmacResult[offset] & 0x7F) << 24) |
		((hmacResult[offset + 1] & 0xFF) << 16) |
		((hmacResult[offset + 2] & 0xFF) << 8) |
		(hmacResult[offset + 3] & 0xFF);

	return binCode % 1000000;
}
bool Verify2FACode(const SecureString& secret, int userCode) {
	long long currentTime = std::time(nullptr);
	long long timeWindow = currentTime / 30;

	// --- ОТЛАДКА НАЧАЛО ---
	qDebug() << "=== ПРОВЕРКА КОДА ===";
	qDebug() << "Время на ПК (UNIX):" << currentTime;
	qDebug() << "Длина секрета в байтах:" << secret.length();

	// Переведем секрет в HEX, чтобы посмотреть, что внутри
	QString hexSecret;
	for (size_t i = 0; i < secret.length(); ++i) {
		hexSecret += QString("%1").arg((unsigned char)secret[i], 2, 16, QChar('0'));
	}
	qDebug() << "Секрет в HEX:" << hexSecret;
	qDebug() << "Код от пользователя:" << userCode;
	qDebug() << "Код, который считает ПК для текущего окна:" << GenerateTOTP(secret, timeWindow);
	qDebug() << "=====================";
	// --- ОТЛАДКА КОНЕЦ ---

	for (int errorRange = -1; errorRange <= 1; errorRange++) {
		if (GenerateTOTP(secret, timeWindow + errorRange) == userCode) {
			return true;
		}
	}
	return false;
}

SecureString GetRandonDEK() {
	unsigned char RawBytes[32];
	if ((RAND_bytes(RawBytes, 32) != 1)) {
		throw std::runtime_error("Не удалось сгенерировать DEK");
	}
	SecureString result(reinterpret_cast<const char*>(RawBytes),sizeof(RawBytes));
	return SecureStringToHex(result);
}
// Перевод сырых байт в читаемый HEX (строку из 0-9, a-f)
SecureString SecureStringToHex(const SecureString input) {
	if (input.empty()) return SecureString();

	const char hexChars[] = "0123456789abcdef";

	// 1. Сразу выделяем под результат правильный объем защищенной памяти
	SecureString hexResult;
	hexResult.resize(input.size() * 2);

	// 2. Быстро и безопасно переводим байты
	for (size_t i = 0; i < input.size(); ++i) {
		unsigned char byte = static_cast<unsigned char>(input[i]);
		hexResult[i * 2] = hexChars[byte >> 4];     // Старший полубайт
		hexResult[i * 2 + 1] = hexChars[byte & 0x0F];   // Младший полубайт
	}

	return hexResult;
}

SecureString HexToSecureString(const SecureString& input) {
	if (input.length() % 2 != 0) return SecureString();

	SecureString output;
	// 1. Сразу выделяем память, чтобы избежать цепочки переаллокаций, 
	// которая сводит с ума отладочные итераторы MSVC
	output.reserve(input.length() / 2);

	for (size_t i = 0; i < input.length(); i += 2) {
		// Заменяем тяжелый substr + std::stol на быстрый и безопасный посимвольный парсинг
		char high = input[i];
		char low = input[i + 1];

		int highVal = (high >= '0' && high <= '9') ? (high - '0') :
			(high >= 'a' && high <= 'f') ? (high - 'a' + 10) :
			(high >= 'A' && high <= 'F') ? (high - 'A' + 10) : -1;

		int lowVal = (low >= '0' && low <= '9') ? (low - '0') :
			(low >= 'a' && low <= 'f') ? (low - 'a' + 10) :
			(low >= 'A' && low <= 'F') ? (low - 'A' + 10) : -1;

		// Если в строке мусор — отдаем пустую строку безопасно для кучи
		if (highVal == -1 || lowVal == -1) return "";
		output.push_back((char)((highVal << 4) | lowVal));
	}
	return output;
}
// Перевод HEX-строки обратно в сырые бинарные байты
std::string HexToString(const std::string& input) {
	if (input.length() % 2 != 0) return "";

	std::string output;
	// 1. Сразу выделяем память, чтобы избежать цепочки переаллокаций, 
	// которая сводит с ума отладочные итераторы MSVC
	output.reserve(input.length() / 2);

	for (size_t i = 0; i < input.length(); i += 2) {
		// Заменяем тяжелый substr + std::stol на быстрый и безопасный посимвольный парсинг
		char high = input[i];
		char low = input[i + 1];

		int highVal = (high >= '0' && high <= '9') ? (high - '0') :
			(high >= 'a' && high <= 'f') ? (high - 'a' + 10) :
			(high >= 'A' && high <= 'F') ? (high - 'A' + 10) : -1;

		int lowVal = (low >= '0' && low <= '9') ? (low - '0') :
			(low >= 'a' && low <= 'f') ? (low - 'a' + 10) :
			(low >= 'A' && low <= 'F') ? (low - 'A' + 10) : -1;

		// Если в строке мусор — отдаем пустую строку безопасно для кучи
		if (highVal == -1 || lowVal == -1) return "";

		output.push_back((char)((highVal << 4) | lowVal));
	}

	return output;
}

std::string CalculateSHA256(const std::string& input) {
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hashLen = 0;

	// Создаем умный указатель для контекста хэширования EVP_MD_CTX
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> mdCtx(
		EVP_MD_CTX_new(),
		EVP_MD_CTX_free
	);

	if (!mdCtx) {
		throw std::runtime_error("Не удалось создать контекст хэширования");
	}

	// Инициализируем хэш-алгоритм SHA-256
	if (EVP_DigestInit_ex(mdCtx.get(), EVP_sha256(), nullptr) != 1) {
		throw std::runtime_error("Ошибка инициализации SHA-256");
	}

	// Хэшируем входные данные
	if (EVP_DigestUpdate(mdCtx.get(), input.c_str(), input.length()) != 1) {
		throw std::runtime_error("Ошибка в ходе хэширования");
	}

	// Финализируем и получаем результат в массив hash
	if (EVP_DigestFinal_ex(mdCtx.get(), hash, &hashLen) != 1) {
		throw std::runtime_error("Ошибка финализации хэша");
	}

	// Переводим полученные байты в HEX-строку
	return StringToHex(std::string(reinterpret_cast<const char*>(hash), hashLen));
}

std::vector<unsigned char> DerivedKey(const SecureString& text, const std::vector<unsigned char>& salt) {
	std::vector<unsigned char> key(32);
	int iterations = 100000;
	if (PKCS5_PBKDF2_HMAC(text.c_str(), text.length(),
		salt.data(), salt.size(),
		iterations,
		EVP_sha256(),
		key.size(), key.data()) != 1) 
	{
		std::fill(key.begin(), key.end(), 0);
		throw std::runtime_error("Ошибка OpenSSL: не удалось сгенерировать ключ через PBKDF2");
	}
	return key;
}

std::string EncryptAES(const SecureString& plainText, const SecureString& password, const std::vector<unsigned char>& salt) {
	unsigned char iv[16];
	if (!(RAND_bytes(iv, 16) == 1)) {
		throw std::runtime_error("Не удалось сгенерировать вспомогательный вектор iv");
	}
	std::vector<unsigned char> key = DerivedKey(password,salt);
	std::vector<unsigned char> encryptedText(plainText.size() + EVP_MAX_BLOCK_LENGTH);
	int Len = 0;
	int CipherLen = 0;
	std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>ctx(
		EVP_CIPHER_CTX_new(),
		EVP_CIPHER_CTX_free
	);
	if (!ctx) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("Не удалось создать ctx");
	}
	if ((EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, key.data(), iv)) != 1) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("Ошибка в ходе инициализации AES");
	}
	if (EVP_EncryptUpdate(ctx.get(), encryptedText.data(), &Len, reinterpret_cast<const unsigned char*>(plainText.data()), plainText.size()) != 1) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("Ошибка в ходе шифрования");
	}
	int FinalLen = 0;
	if (EVP_EncryptFinal_ex(ctx.get(), encryptedText.data() + Len, &FinalLen) != 1) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("Ошибка выравнивания итогового текста");
	}
	CipherLen = Len + FinalLen;
	OPENSSL_cleanse(key.data(), key.size());
	std::string result(reinterpret_cast<const char*>(iv), sizeof(iv));
	result.append(reinterpret_cast<const char*>(encryptedText.data()), CipherLen);
	// Вместо простого return result;
	return StringToHex(result);
}

SecureString DecryptAES(const std::string& encryptedData, const SecureString& password, const std::vector<unsigned char>& salt) {
	std::string EncryptedText = HexToString(encryptedData);
	if (EncryptedText.size() < 16) {
		throw std::runtime_error("Not enough symbols");
	}
	if (password.length() == 0) {
		throw std::runtime_error("Not enough symbols");
	}
	std::vector<unsigned char> key = DerivedKey(password, salt);
	unsigned char IV[16];
	memcpy(IV, EncryptedText.data(), 16);
	int EncryptedTextLen = EncryptedText.size() - 16;
	std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>ctx(
		EVP_CIPHER_CTX_new(),
		EVP_CIPHER_CTX_free
	);
	if (!ctx) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("cant make ctx");
	}
	if ((EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, key.data(), IV)) != 1) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("Error while AES initializing");
	}
	int Len = 0;
	std::vector<unsigned char> DescryptedText(EncryptedTextLen);
	if (EVP_DecryptUpdate(ctx.get(), DescryptedText.data(), &Len, reinterpret_cast<const unsigned char*>(EncryptedText.data()) + 16, EncryptedTextLen) != 1) {
		OPENSSL_cleanse(key.data(), key.size());
		throw std::runtime_error("Error while encrypting");
	}
	int FinalLen = 0;
	if (EVP_DecryptFinal_ex(ctx.get(), DescryptedText.data() + Len, &FinalLen) != 1) {
		OPENSSL_cleanse(key.data(), key.size());
		return SecureString(); // Вместо throw лучше вернуть пустую строку — это предотвратит падение UI
	}
	int TotalLen = FinalLen + Len;
	OPENSSL_cleanse(key.data(), key.size());
	SecureString rawResult(reinterpret_cast<char*>(DescryptedText.data()), TotalLen);
	OPENSSL_cleanse(DescryptedText.data(), DescryptedText.size());
	return rawResult;
}

std::string GenerateRandomSalt() {
	unsigned char buffer[16];
	if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
		throw std::runtime_error("Ошибка OpenSSL: не удалось сгенерировать соль");
	}
	std::string rawBytes(reinterpret_cast<const char*>(buffer), sizeof(buffer));
	return StringToHex(rawBytes); // <-- ИСПРАВЛЕНО: теперь возвращаем чистый HEX текст
}

std::string MerkleRoot(const std::vector<Note>& UserNotes) {
	long long s = UserNotes.size();
	if (s == 0) return "";

	std::vector<std::string> Archieved;

	for (int i = 0; i < s; i++) {
		std::string fullContent = CalculateSHA256(UserNotes[i].Name + '\0' + UserNotes[i].SecuredText);
		Archieved.push_back(fullContent);
	}

	// Шаг 2: Схлопываем дерево попарно
	while (Archieved.size() > 1) {
		if (Archieved.size() % 2 != 0) {
			Archieved.push_back(Archieved.back());
		}
		std::vector<std::string> NewLevel;
		for (size_t i = 0; i < Archieved.size(); i += 2) {
			NewLevel.push_back(CalculateSHA256(Archieved[i]+ Archieved[i + 1]));
		}
		Archieved = NewLevel;
	}
	return Archieved[0];
}

SecureString QStringToSecureString(const QString& qtStr) {
	if (qtStr.isEmpty()) return SecureString();

	// 1. Переводим QString в массив байт (обычно в кодировке UTF-8)
	QByteArray bytes = qtStr.toUtf8();

	// 2. Явно конструируем SecureString из сырого указателя на данные и их длины
	SecureString secureStr(bytes.constData(), bytes.size());

	// 3. ЗАЩИТА ПАМЯТИ: Принудительно затираем временный QByteArray нулями.
	// Это гарантирует, что пароль/текст не останется в куче после уничтожения bytes.
	std::fill(bytes.begin(), bytes.end(), 0);

	return secureStr;
}