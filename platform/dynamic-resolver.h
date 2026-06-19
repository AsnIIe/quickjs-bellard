#pragma once

#ifndef LIBRARYLOADER_H
#define LIBRARYLOADER_H

#include "../cutils.h"

#if defined(_WIN32)

static inline char* dynamic_strreplece(char* buf, char c, char x) {
	size_t strlen = 0;
	while (*buf) {
		if (*buf == c)
			*buf = x;
		buf++;
		strlen++;
	}
	buf -= strlen;
	return buf;
}

static inline int dynamic_strrchr(const char* s, char c) {
	size_t size = strlen(s);
	for (int i = size - 1; i >= 0; i--) {
		if (s[i] == c)
			return i;
	}
	return -1;
}

static inline char* dynamic_substr(const char* s, char* buf, size_t sIdx, size_t size) {
	memset(buf, 0, strlen(s) + 1);
	memcpy(buf, s + sIdx, size);
	return buf;
}

static inline int dynamic_strchr(const char* s, char c) {
	size_t size = strlen(s);
	for (size_t i = 0; i < size; i++) {
		if (s[i] == c)
			return i;
	}
	return -1;
}

static inline char* dynamic_strbetween(const char* str, char* dest, char s, char e) {
	int s_i = -1;
	if (s == e)
		s_i = dynamic_strchr(str, s);
	else
		s_i = dynamic_strrchr(str, s);

	if (s_i == -1)
		s_i = 0;
	else
		s_i += 1;

	int e_i = -1;
	if (s == e)
		e_i = dynamic_strrchr(str + s_i, e);
	else
		e_i = dynamic_strchr(str + s_i, e);

	if (e_i == -1)
		e_i = strlen(str + s_i);
	return dynamic_substr(str, dest, s_i, e_i);
}

/**
 * Resolves a module name to a library path and its init function symbol.
 *
 * @param origin  Module identifier or relative path.
 * @param dir     Base directory containing the target library.
 * @param f       Output buffer for the resolved library path (caller-allocated).
 * @param m       Output buffer for the initialization function name (caller-allocated).
 */
static void dynamic_resolve(const char* origin, const char* dir, char* f, char* m) {
	char path[MAX_PATH] = { 0 };
	char file[MAX_PATH] = { 0 };
	char method[MAX_PATH] = { 0 };

	if (!strstart(origin, "..", NULL)
		&& !strstart(origin, ".", NULL)
		&& !strchr(origin, ':')) {
		if (dir) {
			strcpy(path, dir);
		} else {
			strcpy(path, ".\\");
		}
		strcat(path, origin);
	} else {
		strcpy(path, origin);
	}
	if (!has_suffix(path, ".dll"))
		strcat(path, ".dll");

	dynamic_strreplece(path, '/', '\\');
	int s_i = dynamic_strrchr(path, '\\');
	if (s_i == -1)
		strcpy(f, path);
	else
		dynamic_substr(path, f, 0, (size_t)s_i + 1);

	dynamic_strbetween(path, file, '\\', '.');
	strcat(f, file);
	strcat(f, ".dll");

	int offset = 0;
	if (path[0] == '.' && path[1] == '.')
		offset = 2;
	else if (path[0] == '.')
		offset = 1;

	dynamic_strbetween(path + offset, method, '.', '.');
	if (strcmp("dll", method) == 0)
		sprintf(m, "js_init_%s", file);
	else
		sprintf(m, "js_init_%s_%s", file, method);
	dynamic_strreplece(m, '.', '_');
}

static inline char* dynamic_u82a(const char* content, char* dst) {
	int len = 0;
	unsigned short* ansi = NULL;

	len = MultiByteToWideChar(CP_UTF8, 0, content, -1, NULL, 0);
	ansi = (unsigned short*)malloc(sizeof(unsigned short) * len);
	if (!ansi)
		return NULL;

	MultiByteToWideChar(CP_UTF8, 0, (LPCCH)content, -1, (LPWSTR)ansi, len);
	len = WideCharToMultiByte(CP_ACP, 0, (LPCWCH)ansi, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, (LPCWCH)ansi, -1, dst, len, NULL, NULL);
	dst[len] = '\0';
	free(ansi);
	return dst;
}

static inline BOOL dynamic_IsValidPEFile(const char* module_name) {
	HANDLE hFile = CreateFileA(module_name, GENERIC_READ, FILE_SHARE_READ, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	// Read DOS header
	IMAGE_DOS_HEADER dosHeader;
	DWORD bytesRead;
	if (!ReadFile(hFile, &dosHeader, sizeof(dosHeader), &bytesRead, NULL) ||
		bytesRead != sizeof(dosHeader)) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Check MZ signature
	if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Positioning PE head
	if (SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Read PE signature
	DWORD peSignature;
	if (!ReadFile(hFile, &peSignature, sizeof(peSignature), &bytesRead, NULL) ||
		bytesRead != sizeof(peSignature)) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Check PE signature
	if (peSignature != IMAGE_NT_SIGNATURE) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Continue checking after verifying the PE signature
	IMAGE_FILE_HEADER fileHeader;
	if (!ReadFile(hFile, &fileHeader, sizeof(fileHeader), &bytesRead, NULL) ||
		bytesRead != sizeof(fileHeader)) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Check machine type
	if (fileHeader.Machine != IMAGE_FILE_MACHINE_I386 &&
		fileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
		CloseHandle(hFile);
		return FALSE;
	}
	// Check the optional head size
	if (fileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER32) &&
		fileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64)) {
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}
#else
#error "other types of compilers are not supported yet";
#endif /* !_WIN32 */

#endif /* LIBRARYLOADER_H */
