#pragma once

#include <QDialog>  // Базовый класс для диалоговых окон
#include <QMap>
#include <map>
#include <string>    // Для геттеров, которые будут возвращать std::string наружу
#include "CryptoManager.h"

// Опережающие объявления виджетов Qt, чтобы хедер оставался легким
class QLineEdit;
class QTextEdit;
class QPushButton;
class QCheckBox;
class QHBoxLayout;

class CreateNoteDialog : public QDialog {
    Q_OBJECT
public:
    CreateNoteDialog(QWidget* parent = nullptr);
    ~CreateNoteDialog();

    std::string GetName() const;
    SecureString GetText() const;
    SecureString GetCustomKey() const;
    std::vector<std::string> GetTags() const { return m_CurrentTags; }
    bool IsAutoEncryptChecked() const;
    std::map<std::string, std::string> GetAllAttachments() const;
private slots:
    void HandleSave();
    void HandleCancel();
    void HandleCheckBox(bool checked);
private:
    void UpdateDialogAttachments();
    void UpdateDialogTags();

    QLineEdit* m_AttachmentPathDisplay;
    QPushButton* m_BrowseAttachmentButton;
    QPushButton* m_AddAttachmentButton;
    QHBoxLayout* m_AttachmentsLayout;
    QMap<QString, QString> m_PendingAttachments;

    QLineEdit* m_TagInput; // Строка для ввода одного тега
    QPushButton* m_AddTagButton; // Кнопка "+"
    QHBoxLayout* m_TagsLayout;   // Слой, где будут появляться добавленные теги
    std::vector<std::string> m_CurrentTags;
    QLineEdit* m_NameInput;
    QTextEdit* m_TextInput;
    QCheckBox* m_AutoEncryptCheck;
    QLineEdit* m_CustomKeyInput;
    QPushButton* m_SaveButton;
    QPushButton* m_CancelButton;
};