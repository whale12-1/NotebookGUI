#include "crow.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    crow::SimpleApp app;

#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    // Убедимся, что папка для данных существует (и локально на Win, и в Docker на Linux)
    fs::create_directories("data/user_uploads");

    // 1. Эндпоинт авторизации (твоя рабочая заглушка)
    CROW_ROUTE(app, "/v1/auth/login").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            return crow::response(400, "Invalid JSON");
        }

        std::string username = json_data["username"].s();
        std::string auth_hash = json_data["auth_hash"].s();

        if (auth_hash.length() == 64) {
            crow::json::wvalue response_json;
            response_json["token"] = "SERVER_GENERATED_JWT_TOKEN_EXAMPLE_12345";

            CROW_LOG_INFO << "Пользователь " << username << " успешно авторизован.";
            return crow::response(200, response_json);
        }

        return crow::response(401, "Unauthorized: Invalid hash");
        });

    // 2. Эндпоинт для загрузки монолитного файла базы данных
    CROW_ROUTE(app, "/v1/sync/upload-db").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        // Проверяем авторизацию (базовая проверка заголовка Bearer)
        std::string auth_header = req.get_header_value("Authorization");
        if (auth_header.empty() || auth_header.find("Bearer ") == std::string::npos) {
            return crow::response(401, "Missing or invalid token");
        }

        // Вытаскиваем версию базы данных из кастомного заголовка, который шлет SyncManager
        std::string db_version = req.get_header_value("X-DB-Version");
        CROW_LOG_INFO << "Получена база данных. Версия на клиенте: " << db_version;

        // В реальном приложении мы бы вытащили username из токена,
        // но пока сохраняем как единый бэкап в нашу примонтированную папку
        std::string target_path = "data/user_uploads/remote_SafeData.db";

        // Открываем файл в бинарном режиме для записи
        std::ofstream out_file(target_path, std::ios::binary);
        if (!out_file.is_open()) {
            CROW_LOG_ERROR << "Не удалось создать файл на диске: " << target_path;
            return crow::response(500, "Internal Server Error: Cannot write file");
        }

        // Записываем все бинарные данные из тела запроса (req.body)
        out_file.write(req.body.data(), req.body.size());
        out_file.close();

        CROW_LOG_INFO << "Файл базы данных успешно сохранен. Размер: " << req.body.size() << " байт.";
        return crow::response(200, "DB uploaded successfully");
        });

    // Запускаем сервер на порту 8080
    app.port(8080).multithreaded().run();
}