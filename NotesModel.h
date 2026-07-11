#pragma once
#include <QAbstractListModel>
#include <vector>
#include "CryptoManager.h" // Твоя структура Note
#include <openssl/crypto.h>
#include "DatabaseManager.h"

class NotesModel : public QAbstractListModel {
    Q_OBJECT

private:
    std::vector<Note>* m_Notes; // Указатель на реальный вектор заметок в MainWindow

public:
    void PopulateFromDatabase() {
        // Сообщаем интерфейсу: "Внимание, сейчас список данных полностью изменится!"
        beginResetModel();

        // Забираем из БД свежий вектор заметок со всеми ID и тегами
        *m_Notes = DatabaseManager::instance().getAllNotes();

        // Сообщаем интерфейсу: "Готово, перерисовывай экран"
        endResetModel();
    }
    // Конструктор принимает указатель на вектор, чтобы работать с ним напрямую
    NotesModel(std::vector<Note>* notes, QObject* parent = nullptr)
        : QAbstractListModel(parent), m_Notes(notes) {
    }
    std::vector<Note> GetNotesVector() { return *m_Notes; }
    bool AddNote(Note& note) {
        if (!m_Notes) return false;

        // 1. Пытаемся сохранить в базу данных. 
        if (DatabaseManager::instance().AddNote(note)) {

            // 2. Уведомляем Qt, что мы добавляем одну строку в конец списка
            // ИСПРАВЛЕНО: заменено на -> так как m_Notes — это указатель
            beginInsertRows(QModelIndex(), m_Notes->size(), m_Notes->size());

            // 3. Добавляем объект с уже заполненным ID в наш внутренний вектор
            // ИСПРАВЛЕНО: заменено на ->
            m_Notes->push_back(note);

            endInsertRows();
            return true;
        }
        return false;
    }
    bool RemoveNote(int row) {
        if (!m_Notes || row < 0 || row >= static_cast<int>(m_Notes->size())) return false;
        long long noteId = (*m_Notes)[row].Id;

        // 1. Удаляем из базы данных
        if (DatabaseManager::instance().RemoveNote(noteId)) {

            // 2. Удаляем из интерфейса
            // ИСПРАВЛЕНО: заменено на -> во всех операциях с вектором
            beginRemoveRows(QModelIndex(), row, row);
            m_Notes->erase(m_Notes->begin() + row);
            endRemoveRows();

            return true;
        }
        return false;
    }
    // 1. Возвращает количество элементов в списке (Qt вызывает его сам)
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        if (!m_Notes) return 0;
        return static_cast<int>(m_Notes->size());
    }

    // 2. Возвращает данные для отображения конкретной строки (Qt вызывает его сам)
    QVariant data(const QModelIndex& index, int role) const override {
        // Проверяем валидность индекса строки и существование вектора
        if (!index.isValid() || !m_Notes) return QVariant();

        int row = index.row();
        if (row < 0 || row >= static_cast<int>(m_Notes->size())) return QVariant();

        // Если Qt запрашивает текст для отображения на экране
        if (role == Qt::DisplayRole) {
            // Получаем имя конкретной заметки из вектора
            QString noteName = QString::fromStdString((*m_Notes)[row].Name);

            // Полностью повторяем твою логику форматирования: [номер] имя
            return QString("[%1] %2").arg(row + 1).arg(noteName);
        }

        return QVariant();
    }
};