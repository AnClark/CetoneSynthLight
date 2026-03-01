#include "PresetManager.h"
#include "Defines.h"

#ifdef DISTRHO_OS_WINDOWS
#include <shlobj.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <cstdio>
#include <cstring>
#include <string>

// ============================================================================
// File System Helpers
// ============================================================================

bool CPresetManager::_fileExists(const String& path) const {
    if (path.isEmpty())
        return false;
#ifdef DISTRHO_OS_WINDOWS
    DWORD attrib = GetFileAttributesA(path.buffer());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path.buffer(), &st) == 0 && S_ISREG(st.st_mode));
#endif
}

bool CPresetManager::_createDirectoryIfNeeded(const String& path) const {
    if (path.isEmpty())
        return false;

#ifdef DISTRHO_OS_WINDOWS
    DWORD attrib = GetFileAttributesA(path.buffer());
    if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY))
        return true;  // Already exists

    // Attempt to create recursively
    std::string pathStr(path.buffer());
    // Build all intermediate directories
    for (size_t i = 1; i < pathStr.size(); i++) {
        if (pathStr[i] == '\\' || pathStr[i] == '/') {
            std::string sub = pathStr.substr(0, i);
            CreateDirectoryA(sub.c_str(), nullptr);
        }
    }
    if (!CreateDirectoryA(path.buffer(), nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS)
            return true;
        d_stderr("_createDirectoryIfNeeded: Failed to create directory '%s' (error %lu)",
                 path.buffer(), (unsigned long)err);
        return false;
    }
    return true;
#else
    struct stat st;
    if (stat(path.buffer(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    std::string pathStr(path.buffer());
    for (size_t i = 1; i < pathStr.size(); i++) {
        if (pathStr[i] == '/') {
            std::string sub = pathStr.substr(0, i);
            mkdir(sub.c_str(), 0755);
        }
    }
    if (mkdir(path.buffer(), 0755) != 0) {
        if (errno == EEXIST)
            return true;
        d_stderr("_createDirectoryIfNeeded: Failed to create directory '%s'", path.buffer());
        return false;
    }
    return true;
#endif
}

String CPresetManager::_readFileContent(const String& filePath) const {
    if (filePath.isEmpty())
        return String();

    FILE* f = fopen(filePath.buffer(), "rb");
    if (!f) {
        d_stderr("_readFileContent: Cannot open file '%s'", filePath.buffer());
        return String();
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0) {
        fclose(f);
        d_stderr("_readFileContent: File is empty or error: '%s'", filePath.buffer());
        return String();
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize) + 1, '\0');
    size_t bytesRead = fread(buffer.data(), 1, static_cast<size_t>(fileSize), f);
    fclose(f);

    if (bytesRead == 0)
        return String();

    return String(buffer.data());
}

bool CPresetManager::_writeFileContent(const String& filePath,
                                        const String& content) const {
    if (filePath.isEmpty())
        return false;

    FILE* f = fopen(filePath.buffer(), "wb");
    if (!f) {
        d_stderr("_writeFileContent: Cannot open file '%s' for writing", filePath.buffer());
        return false;
    }

    size_t len = content.length();
    size_t written = fwrite(content.buffer(), 1, len, f);
    fclose(f);

    if (written != len) {
        d_stderr("_writeFileContent: Write incomplete for '%s'", filePath.buffer());
        return false;
    }

    return true;
}

// ============================================================================
// Bank Path Helpers
// ============================================================================

String CPresetManager::_getDefaultBankPath() const {
    String dir = _getUserPresetsDirectory();
    if (dir.isEmpty())
        return String();

#ifdef DISTRHO_OS_WINDOWS
    return dir + "\\" + String(DEFAULT_USER_BANK_FILENAME);
#else
    return dir + "/" + String(DEFAULT_USER_BANK_FILENAME);
#endif
}

String CPresetManager::_getBanksDirectory() const {
    String baseDir = _getUserPresetsDirectory();
    if (baseDir.isEmpty())
        return String();

#ifdef DISTRHO_OS_WINDOWS
    return baseDir + "\\" + String(USER_PRESET_BANK_SUBDIR);
#else
    return baseDir + "/" + String(USER_PRESET_BANK_SUBDIR);
#endif
}

String CPresetManager::_getBankFilePath(const char* bankName) const {
    if (!bankName || bankName[0] == '\0')
        return String();

    String banksDir = _getBanksDirectory();
    if (banksDir.isEmpty())
        return String();

#ifdef DISTRHO_OS_WINDOWS
    return banksDir + "\\" + String(bankName) + String(USER_PRESET_BANK_EXTENSION);
#else
    return banksDir + "/" + String(bankName) + String(USER_PRESET_BANK_EXTENSION);
#endif
}

bool CPresetManager::_isDefaultUserBank(const char* bankName) const {
    if (!bankName || bankName[0] == '\0')
        return false;

    return std::strcmp(bankName, DEFAULT_USER_BANK_NAME) == 0;
}

bool CPresetManager::_isFactoryBank(const char* bankName) const {
    if (!bankName || bankName[0] == '\0')
        return false;

    return std::strcmp(bankName, FACTORY_BANK_NAME) == 0;
}

void CPresetManager::_sanitizeBankName(String& bankName) const {
    if (bankName.isEmpty())
        return;

    // Replace characters invalid in file names
    std::string s(bankName.buffer());
    const char* invalidChars = "/\\:*?\"<>|";
    for (size_t i = 0; i < s.size(); i++) {
        for (const char* p = invalidChars; *p; p++) {
            if (s[i] == *p) {
                s[i] = '_';
                break;
            }
        }
    }

    // Remove leading/trailing whitespace
    size_t start = s.find_first_not_of(" \t");
    size_t end   = s.find_last_not_of(" \t");
    if (start == std::string::npos)
        s = "Bank";
    else
        s = s.substr(start, end - start + 1);

    if (s.empty())
        s = "Bank";

    bankName = String(s.c_str());
}

// ============================================================================
// Internal Bank File I/O
// ============================================================================

bool CPresetManager::_loadBankFromFile(const String& filePath, PresetBank& outBank) const {
    if (!_fileExists(filePath)) {
        d_stderr("_loadBankFromFile: File not found: '%s'", filePath.buffer());
        return false;
    }

    String jsonContent = _readFileContent(filePath);
    if (jsonContent.isEmpty()) {
        d_stderr("_loadBankFromFile: Empty content from: '%s'", filePath.buffer());
        return false;
    }

    if (!deserializeBankFromJSON(jsonContent, outBank)) {
        d_stderr("_loadBankFromFile: Deserialization failed for: '%s'", filePath.buffer());
        return false;
    }

    return true;
}

bool CPresetManager::_saveBankToFile(const String& filePath, const PresetBank& bank) const {
    String jsonContent = serializeBankToJSON(bank);
    if (jsonContent.isEmpty()) {
        d_stderr("_saveBankToFile: Failed to serialize bank '%s'", bank.Name.buffer());
        return false;
    }

    if (!_writeFileContent(filePath, jsonContent)) {
        d_stderr("_saveBankToFile: Failed to write file '%s'", filePath.buffer());
        return false;
    }

    d_stderr("_saveBankToFile: Saved bank '%s' to '%s'", bank.Name.buffer(), filePath.buffer());
    return true;
}
