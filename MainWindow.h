#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <vector>
#include <string>
#include "CryptoManager.h" // Предположим, структура Note лежит тут
#include "FileManager.h"
#include <openssl/crypto.h>
#include "NotesModel.h"
#include "CustomFilterProxyModel.h"
#include "ProtectedKey.h"
#include "TwoFactorDialog.h"
#include <memory>
// Опережающее объявление классов Qt (чтобы не делать кучу #include в хедере)
class QListWidget;
class QPushButton;
class QTimer;
class QString;
class QListView;
class QHBoxLayout;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void InitSession(const SecureString& DecryptedDek,
		const std::string& EncryptedDek,
		const std::string& Salt,
		const SecureString& MasterPassword,
		const std::string& DekHash
	);
private slots:
	void HandleAddNote();
	void HandleBtnSettingsClicked();
	void HandleDecryptNote();
	void HandleDeleteNote();
	void HandleChangeMasterPassword();
	void HandleInactivityLockOut();
	void ClearSecureClipboard();
	void OnCopySecretClicked(const QString& secretText);

private:
	//void RefreshNoteList();
	void UpdateTagsPanel();
	QLineEdit* m_SearchField;
	CustomFilterProxyModel* m_ProxyModel;
	QHBoxLayout* m_TagsLayout;
	QTimer* m_ClipboardTimer;
	QString m_LastCopiedSecret; 
	QListView* m_NotesView;
	NotesModel* m_NotesModel;
	QPushButton* m_CreateButton;
	QPushButton* m_DecryptButton;
	QPushButton* m_DeleteButton;
	QPushButton* m_ChangePassButton;
	QPushButton* m_SettingsBtn;

	QTimer* m_InactivityTimer;
	FileManager m_FileManager;
	std::vector<Note> m_UserNotes;
	std::unique_ptr<ProtectedKey> m_DecryptedDEK;
	std::unique_ptr<ProtectedKey> m_MasterPassword;
	std::string m_DEKHash;
	/*std::string m_ExpectedMerkleRoot;*/
	std::string m_ActualEncryptedDEK;
	std::string m_RandomSalt;
protected:
	bool event(QEvent* event) override;
};
