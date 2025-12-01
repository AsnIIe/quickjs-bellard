#pragma once

#ifndef LIBRARYLOADER_H
#define LIBRARYLOADER_H

#include "../cutils.h"

#if defined(_WIN32)

#define JS_MODULE_PATH ".\\libs\\"

static inline char* dynamic_strreplece(char* buf, char c, char x) {
    size_t strlen = 0;
    while (*buf) {
        if (*buf == c) {
            *buf = x;
        }
        buf++;
        strlen++;
    }
    buf -= strlen;
    return buf;
}

static inline int dynamic_strrchr(const char* s, char c) {
    size_t size = strlen(s);
    for (int i = size - 1; i >= 0; i--) {
        if (s[i] == c) {
            return i;
        }
    }
    return 0;
}

static inline int dynamic_strchr(const char* s, char c) {
    size_t size = strlen(s);
    for (size_t i = 0; i < size; i++) {
        if (s[i] == c) {
            return i;
        }
    }
    return size;
}

static char* dynamic_resolve_path(const char* origin, const char* library, char* dest) {
    if (!strstart(origin, "..", NULL)
        && !strstart(origin, ".", NULL)
        && !strchr(origin, ':')) {
        if (library) {
            strcpy(dest, library);
        } else {
            strcpy(dest, ".\\");
        }
        strcat(dest, origin);
    } else {
        strcpy(dest, origin);
    }
    dynamic_strreplece(dest, '/', '\\');
    if (!has_suffix(dest, ".dll")) {
        strcat(dest, ".dll");
    }
    int sIdx1 = dynamic_strrchr(dest, '\\');
    int sIdx2 = dynamic_strchr(dest + sIdx1, '.');
    dest[sIdx1 + sIdx2] = '\0';
    strcat(dest, ".dll");

    return dest;
}

static char* dynamic_resolve(const char* origin, const char* library, char* dest, char* method) {
    if (!strstart(origin, "..", NULL)
        && !strstart(origin, ".", NULL)
        && !strchr(origin, ':')) {
        if (library) {
            strcpy(dest, library);
        } else {
            strcpy(dest, ".\\");
        }
        strcat(dest, origin);
    } else {
        strcpy(dest, origin);
    }
    dynamic_strreplece(dest, '/', '\\');
    if (!has_suffix(dest, ".dll")) {
        strcat(dest, ".dll");
    }

    {
        strcpy(method, "js_init_");

        int sIdx = 0, eIdx = 0, size = 0;
        sIdx = dynamic_strrchr(dest, '\\');
        eIdx = dynamic_strrchr(dest, '.');
        size = eIdx - sIdx - 1;
        memcpy(method + 8, dest + sIdx + 1, size);

        dynamic_strreplece(method, '.', '_');
    }

    int sIdx1 = dynamic_strrchr(dest, '\\');
    int sIdx2 = dynamic_strchr(dest + sIdx1, '.');
    dest[sIdx1 + sIdx2] = '\0';
    strcat(dest, ".dll");

    return dest;
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
    char filePath[MAX_PATH] = { 0 };
    char filePathU8[MAX_PATH] = { 0 };
    dynamic_resolve_path(module_name, JS_MODULE_PATH, filePathU8);

    dynamic_u82a(filePathU8, filePath);

    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
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
