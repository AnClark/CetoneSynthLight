#pragma once

#include "extra/String.hpp" // For DPF String class
#include "Defines.h"
#include "Structures.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

// Forward declaration to avoid circular dependency
class CCetoneUI;

struct PresetBank {
    String                   Name;
    std::vector<SynthProgram> Presets;
};

class CPresetManager {
public:
    CPresetManager(CCetoneUI* ui)
        : ui(ui)
    {
        initFactoryPrograms(); // Load factory preset data into memory first
        // loadDefaultBank() is called explicitly from CCetoneUI constructor
    }
    ~CPresetManager() { }

    // -------------------------------------------------------------------
    // Operations on programs (aka. presets)

    void loadProgram(const SynthProgram& program);
    void loadDefaultProgram();

    // -------------------------------------------------------------------
    // Operations on factory banks

protected:
    void   initFactoryPrograms();
public:
    String getFactoryProgramName(uint32_t index) const;
    int    getFactoryProgramCount() const { return static_cast<int>(FactoryPrograms.size()); }
    void   loadFactoryProgram(uint32_t index);

    // -------------------------------------------------------------------
    // JSON Serialization for User Preset Banks

    String serializeBankToJSON(const PresetBank& bank) const;
    bool   deserializeBankFromJSON(const String& jsonString, PresetBank& outBank) const;

    // Single Preset Import/Export
    String serializePresetToJSON(const SynthProgram& preset) const;
    bool   deserializePresetFromJSON(const String& jsonString, SynthProgram& outPreset) const;
    bool   exportCurrentPresetToFile(const char* filePath);
    bool   importPresetFromFile(const char* filePath, String* outPresetName = nullptr);

    // -------------------------------------------------------------------
    // Default User Preset Bank Management

    bool   loadDefaultBank();
    bool   saveDefaultBank();
    void   savePresetToDefaultBank(const char* presetName, const SynthProgram& preset);
    bool   loadPresetFromDefaultBank(const char* presetName, SynthProgram& outPreset);
    bool   deletePresetFromDefaultBank(const char* presetName);
    bool   renamePresetInDefaultBank(const char* oldName, const char* newName);
    String getDefaultBankName() const;
    size_t getDefaultBankPresetCount() const;
    String getDefaultBankPresetName(size_t index) const;
    bool   loadPresetFromDefaultBank(const char* presetName); // Load from Default User Bank and update State

    // -------------------------------------------------------------------
    // Bank Management (Multi-Bank Support)

    std::vector<String>   getImportedBankNames();
    bool                  importBankFromFile(const char* filePath);
    bool                  exportBankToFile(const char* bankName, const char* filePath);
    bool                  deleteBankByName(const char* bankName);
    bool                  renameBankByName(const char* oldName, const char* newName);
    bool                  loadPresetFromBank(const char* bankName, const char* presetName);
    std::vector<String>   getPresetsInBank(const char* bankName);

    // Preset-level operations within Banks
    bool                  createNewBank(const char* bankName);
    bool                  savePresetToBank(const char* bankName, const char* presetName, const SynthProgram& preset);
    bool                  deletePresetFromBank(const char* bankName, const char* presetName);
    bool                  renamePresetInBank(const char* bankName, const char* oldName, const char* newName);

    // -------------------------------------------------------------------
    // Parameter Capture

    SynthProgram captureCurrentParameters() const;

private:
    CCetoneUI*   ui; // UI instance
    PresetBank   fDefaultUserBank; // Default User Preset Bank
    std::vector<SynthProgram> FactoryPrograms; // Factory preset data (in memory)

    // -------------------------------------------------------------------
    // Inner Helpers

    void _triggerParamUpdate(uint32_t paramId, float newValue);

    // File I/O helpers
    String _getDefaultBankPath() const;
    String _getUserPresetsDirectory() const;
    bool   _fileExists(const String& path) const;
    String _readFileContent(const String& path) const;
    bool   _writeFileContent(const String& path, const String& content) const;
    bool   _createDirectoryIfNeeded(const String& dirPath) const;

    // Bank management helpers
    String _getBanksDirectory() const;
    String _getBankFilePath(const char* bankName) const;
    bool   _loadBankFromFile(const String& filePath, PresetBank& outBank) const;
    bool   _saveBankToFile(const String& filePath, const PresetBank& bank) const;
    bool   _isDefaultUserBank(const char* bankName) const;
    bool   _isFactoryBank(const char* bankName) const;

    // Utilities
    void _sanitizeBankName(String& bankName) const;
};
