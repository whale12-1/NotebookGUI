#include <QApplication>
#include "AuthWindow.h"
#include <QFile>
#include <QTextStream>
int main(int argc, char* argv[]) {
    // Ensuring correct UTF-8 configuration for environments like Apple Clang / macOS
#if defined(__APPLE__)
// Дополнительные настройки локали при необходимости, 
// Qt 6 по умолчанию отлично работает с UTF-8
#endif
    QApplication a(argc, argv);
    // Загружаем стили глобально для ВСЕГО приложения
    QFile styleFile(":/Style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&styleFile);
        a.setStyleSheet(ts.readAll()); // Применяем к 'a' (QApplication)
        styleFile.close();
    }
    // Создаем окно авторизации на стеке, так как оно является точкой входа
    AuthWindow authWindow;
    authWindow.show();

    // Запускаем главный цикл обработки событий Qt
    return a.exec();
}
