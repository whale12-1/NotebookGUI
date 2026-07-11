#pragma once

#include <QDialog>
#include <string>

#include "CryptoManager.h"

// Опережающие объявления виджетов Qt (не перегружаем хедер лишними инклудами)
class QLineEdit;
class QPushButton;
class QLabel;

class ChangePassDialog : public QDialog {
	Q_OBJECT
public:
	ChangePassDialog(QWidget* parent = nullptr);
	~ChangePassDialog();

	SecureString GetOldPass() const;
	SecureString GetNewPass() const;
private slots:
	void HandleBtnSave();
	void HandleBtnCancel();
private:
	QLineEdit* m_OldPassInput;
	QLineEdit* m_NewPassInput;
	QLineEdit* m_ConfirmPassInput;
	QPushButton* m_BtnSave;
	QPushButton* m_BtnCancel;
};