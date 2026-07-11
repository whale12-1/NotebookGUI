#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class TwoFactorAuthDialog : public QDialog {
    Q_OBJECT

private:
    QLineEdit* m_CodeField;
    QPushButton* m_ConfirmBtn;
    QPushButton* m_CancelBtn;

public:
    explicit TwoFactorAuthDialog(QWidget* parent = nullptr);

    // Метод для получения введенного кода в виде числа
    int GetEnteredCode() const;
};