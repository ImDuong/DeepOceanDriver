#pragma once

// this templates come from Windows Kernel Programming Book
// github: https://github.com/zodiacon/windowskernelprogrammingbook/blob/master/chapter10/DelProtect3/kstring.cpp

class csstring final {
public:
	explicit csstring(const wchar_t* str = nullptr, POOL_TYPE pool = PagedPool, ULONG tag = 0);
	csstring(const wchar_t* str, ULONG count, POOL_TYPE pool = PagedPool, ULONG tag = 0);
	csstring(const csstring& other);
	explicit csstring(PCUNICODE_STRING str, POOL_TYPE pool = PagedPool, ULONG tag = 0);
	csstring& operator= (const csstring& other);
	csstring(csstring&& other);
	csstring& operator=(csstring&& other);

	~csstring();

	csstring& operator+=(const csstring& other);
	csstring& operator+=(PCWSTR str);

	bool operator==(const csstring& other);

	operator const wchar_t* () const {
		return m_str;
	}

	const wchar_t* Get() const {
		return m_str;
	}

	ULONG Length() const {
		return m_Len;
	}

	csstring ToLower() const;
	csstring& ToLower();
	csstring& Truncate(ULONG length);
	csstring& Append(PCWSTR str, ULONG len = 0);

	void Release();

	inline const wchar_t GetAt(size_t index) const;

	wchar_t& GetAt(size_t index);

	const wchar_t operator[](size_t index) const {
		return GetAt(index);
	}

	wchar_t& operator[](size_t index) {
		return GetAt(index);
	}

	UNICODE_STRING* GetUnicodeString(PUNICODE_STRING);

private:
	wchar_t* Allocate(size_t chars, const wchar_t* src = nullptr);

private:
	wchar_t* m_str;
	ULONG m_Len, m_Capacity;
	POOL_TYPE m_Pool;
	ULONG m_Tag;
};