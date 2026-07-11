#pragma once

#include <QDialog>
#include <string>
#include "CryptoManager.h"
// Опережающие объявления (Forward declarations) виджетов Qt
class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;

class DecryptNoteDialog : public QDialog {
    Q_OBJECT

public:
    DecryptNoteDialog(QWidget* parent = nullptr);
    ~DecryptNoteDialog();

    SecureString GetExtraKey() const;
    bool IsExtraKeyNeeded();
private slots:
    void HandleBtnCustomKey();
    void HandleBtnMasterKey();
    void HandleBtnCancel();
private:
    bool m_IsExtraKeyNeeded;
    QLineEdit* m_KeyInput;
    QPushButton* m_BtnCustomKey;
    QPushButton* m_BtnMasterKey;
    QPushButton* m_BtnCancel;
};