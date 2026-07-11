#pragma once
#include <windows.h>
#include <openssl/crypto.h> 
#include "SecureString.h"
class ProtectedKey {
private:
	SecureString m_Key;
public:
	explicit ProtectedKey(const SecureString& str) : m_Key(str){
		if (!m_Key.empty()) {
			VirtualLock(&m_Key[0], m_Key.capacity());
		}
	}
	~ProtectedKey() {
		if (!m_Key.empty()) {
			VirtualUnlock(&m_Key[0], m_Key.capacity());
			// 2. Только после этого затираем данные нулями
			OPENSSL_cleanse(&m_Key[0], m_Key.length());
		}
	}
	const SecureString& data() const{
		return m_Key;
	}
	ProtectedKey(const ProtectedKey&) = delete;
	ProtectedKey& operator=(const ProtectedKey&) = delete;

	// Разрешаем перемещение (move-семантику), если нужно передать объект
	ProtectedKey(ProtectedKey&& other) noexcept = default;
	ProtectedKey& operator=(ProtectedKey&& other) noexcept = default;
};