#include "CustomFilterProxyModel.h"
#include "NotesModel.h" // Подключи свою модель заметок
#include "CryptoManager.h" // Здесь лежит структура Note
#include <openssl/crypto.h>

CustomFilterProxyModel::CustomFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
}

void CustomFilterProxyModel::setSelectedTag(const QString& tag) {
    m_SelectedTag = tag;
    invalidateFilter(); // Заставляем Qt мгновенно перерисовать список с новым фильтром
}

bool CustomFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    // 1. Сначала проверяем стандартный текстовый поиск (поиск по названию заметки)
    // Метод filterRegExp() или базовые проверки Qt
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    QString noteName = sourceModel()->data(index, Qt::DisplayRole).toString();

    // Если в строке поиска что-то введено, и оно НЕ совпадает с именем, скрываем
    if (!noteName.contains(QSortFilterProxyModel::filterRegularExpression())) {
        return false;
    }

    // 2. ТЕПЕРЬ ФИЛЬТРУЕМ ПО ТЕГУ
    // Если тег не выбран (например, нажата кнопка "Все"), то показываем заметку
    if (m_SelectedTag.isEmpty() || m_SelectedTag == "Все") {
        return true;
    }

    // Кастуем sourceModel к твоей модели, чтобы залезть в вектор заметок
    // (Или делаем это через кастомную роль в data(), но так быстрее для понимания)
    auto* notesModel = qobject_cast<NotesModel*>(sourceModel());
    if (notesModel) {
        // Получаем доступ к вектору заметок напрямую или через твой геттер
        const auto& notes = notesModel->GetNotesVector(); // Убедись, что такой геттер есть
        if (source_row < static_cast<int>(notes.size())) {
            const Note& note = notes[source_row];

            // Ищем, есть ли наш m_SelectedTag среди тегов заметки
            for (const auto& tag : note.Tags) {
                if (QString::fromStdString(tag) == m_SelectedTag) {
                    return true; // Нашли тег! Заметка проходит фильтр
                }
            }
        }
    }

    return false; // Тег не совпал, скрываем строку
}