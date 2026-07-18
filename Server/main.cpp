#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// 2. Сначала подключаем стандартные системные библиотеки C++
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

// 3. Подключаем OpenSSL
#include <openssl/sha.h>

// 4. И только в самом конце подключаем Crow
#include "crow.h"


// Быстрая функция проверки хэша (аналог CalculateSHA256 на клиенте)
std::string CalculateSHA256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int main() {
    crow::SimpleApp app;
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    // Описываем маршрут для авторизации (POST /v1/auth/login)
    CROW_ROUTE(app, "/v1/auth/login").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            return crow::response(400, "Invalid JSON");
        }

        // Извлекаем параметры, присланные SyncManager
        std::string username = json_data["username"].s();
        std::string auth_hash = json_data["auth_hash"].s();

        // НАСТОЯЩАЯ БЕЗОПАСНОСТЬ: 
        // В идеале здесь мы лезем в PostgreSQL, ищем пользователя по username
        // и сверяем присланный auth_hash с тем, что хранится в БД.

        // Пока сделаем заглушку-проверку (сервер одобряет любой хэш правильной длины)
        if (auth_hash.length() == 64) {
            crow::json::wvalue response_json;

            // В продакшене тут генерируется реальный криптографический JWT токен.
            // Для теста подпишем фиксированную строку.
            response_json["token"] = "SERVER_GENERATED_JWT_TOKEN_EXAMPLE_12345";

            CROW_LOG_INFO << "Пользователь " << username << " успешно авторизован.";
            return crow::response(200, response_json);
        }

        return crow::response(401, "Unauthorized: Invalid hash");
        });

    // Запускаем сервер на порту 8080
    app.port(8080).multithreaded().run();
}