#include "ImGui_UI.hpp"

#include "Structures.h"
#include "Defines.h"

#include "CetoneUI.hpp" // For class CCetoneUI

void ImGuiUI::_handlePresetModalPopupRequests()
{
    if (requestRenamePresetPopup)
    {
        ImGui::OpenPopup("Rename Preset");
        requestRenamePresetPopup = false;
    }
    
    if (requestSavePresetPopup)
    {
        ImGui::OpenPopup("Save Preset");
        requestSavePresetPopup = false;
    }
    
    if (requestDeletePresetPopup)
    {
        ImGui::OpenPopup("Delete Preset");
        requestDeletePresetPopup = false;
    }

    if (requestNewBankPopup)
    {
        ImGui::OpenPopup("New Bank");
        requestNewBankPopup = false;
    }

    if (requestRenameBankPopup)
    {
        ImGui::OpenPopup("Rename Bank");
        requestRenameBankPopup = false;
    }

    if (requestDeleteBankPopup)
    {
        ImGui::OpenPopup("Delete Bank");
        requestDeleteBankPopup = false;
    }

    if (requestConfirmOverwritePresetPopup)
    {
        ImGui::OpenPopup("Overwrite Preset?");
        requestConfirmOverwritePresetPopup = false;
    }
}

void ImGuiUI::_buildPresetManagementMenu()
{
    if (ImGui::BeginPopup("Preset Menu"))
    {
        ImGui::SeparatorText("Preset Bank");
        {
            // Factory Presets (built-in, read-only)
            if (ImGui::BeginMenu("Factory Presets"))
            {
                if (ui->fPresetManager->getFactoryProgramCount() == 0)
                {
                    ImGui::MenuItem("(No presets)", nullptr, false, false);
                }
                else
                {
                    for (uint32_t i = 0; i < ui->fPresetManager->getFactoryProgramCount(); i++)
                    {
                        const String presetNameLabel = ui->fPresetManager->getFactoryProgramName(i)
                                                    + String("##FactoryPreset") + String(i);
                        if (ImGui::MenuItem(presetNameLabel.buffer()))
                        {
                            ui->fPresetManager->loadFactoryProgram(i);
                            ui->_updateState(ui->fPresetManager->getFactoryProgramName(i).buffer(),
                                            FACTORY_BANK_NAME, false);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            // Default Preset (shortcut to load Init Patch)
            if (ImGui::MenuItem("Default Preset"))
            {
                // The first patch in Factory Preset list is the Init Patch
                ui->fPresetManager->loadFactoryProgram(0);
                ui->_updateState(ui->fPresetManager->getFactoryProgramName(0).buffer(),
                                 FACTORY_BANK_NAME, false);

                // The way below is OK as well: explicitly load the Init Patch (aka. default program)
                // Now I prefer the first method.
                // ui->fPresetManager->loadDefaultProgram();
                // ui->_updateState(DEFAULT_PRESET_NAME, FACTORY_BANK_NAME, false);
            }

            // Default Bank submenu
            if (ImGui::BeginMenu(DEFAULT_USER_BANK_NAME))
            {
                size_t userPresetCount = ui->fPresetManager->getDefaultBankPresetCount();
                
                if (userPresetCount == 0)
                {
                    ImGui::MenuItem("(No presets)", nullptr, false, false);
                }
                else
                {
                    for (size_t i = 0; i < userPresetCount; i++)
                    {
                        String presetName = ui->fPresetManager->getDefaultBankPresetName(i);
                        const String presetNameLabel = presetName + String("##DefaultBank") + String((int)i);
                        
                        if (ImGui::MenuItem(presetNameLabel.buffer()))
                        {
                            if (ui->fPresetManager->loadPresetFromDefaultBank(presetName.buffer()))
                            {
                                ui->_updateState(presetName.buffer(), DEFAULT_USER_BANK_NAME, false);
                            }
                        }
                    }
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Save Current as..."))
                {
                    strncpy(_presetNameEditorBuffer, ui->fCurrentPresetName.buffer(),
                            MAX_PRESET_NAME_LENGTH);
                    _presetNameEditorBuffer[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                    _pendingSaveBankName = DEFAULT_USER_BANK_NAME;
                    requestSavePresetPopup = true;
                }
                
                // Only enable "Overwrite Current" when the currently loaded preset
                // actually belongs to Default Bank
                const bool isCurrentInDefaultBank =
                    (ui->fCurrentPresetBank == DEFAULT_USER_BANK_NAME);
                if (ImGui::MenuItem("Overwrite Current", nullptr, false, isCurrentInDefaultBank))
                {
                    strncpy(_pendingSavePresetName, ui->fCurrentPresetName.buffer(),
                            MAX_PRESET_NAME_LENGTH);
                    _pendingSavePresetName[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                    _pendingSaveBankName = DEFAULT_USER_BANK_NAME;
                    requestConfirmOverwritePresetPopup = true;
                    requestConfirmOverwritePresetType =
                        RequestConfirmOverwriteType::kOverwriteCurrentPreset;
                }
                
                ImGui::Separator();

                if (ImGui::MenuItem("Export Bank...")) {
                    _fileBrowserBankName = DEFAULT_USER_BANK_NAME;
                    DGL_NAMESPACE::FileBrowserOptions opts;
                    opts.saving = true;
                    opts.title = "Export Default Bank";
                    opts.defaultName = DEFAULT_USER_BANK_FILENAME;
                    if (_fileBrowserHandle != nullptr)
                        DGL_NAMESPACE::fileBrowserClose(_fileBrowserHandle);
                    _fileBrowserHandle = DGL_NAMESPACE::fileBrowserCreate(
                        false, getWindow().getNativeWindowHandle(), getScaleFactor(), opts);
                    _fileBrowserAction = kFileBrowserExportBank;
                }

                ImGui::Separator();
                
                String bankInfo = String("Bank: ") + ui->fPresetManager->getDefaultBankName();
                ImGui::MenuItem(bankInfo.buffer(), nullptr, false, false);
                
                ImGui::EndMenu();
            }


            // Dynamically loaded Banks (imported by user)
            if (_shouldRefreshBankList) {
                importedBanks = ui->fPresetManager->getImportedBankNames();
                importedBankPresets.clear();
                for (const auto& bank : importedBanks)
                    importedBankPresets[bank.buffer()] =
                        ui->fPresetManager->getPresetsInBank(bank.buffer());
                _shouldRefreshBankList = false;
            }

            for (const auto& bankName : importedBanks) {
                const std::vector<String>& presets = importedBankPresets[bankName.buffer()];
                
                char labelBuf[256];
                std::snprintf(labelBuf, sizeof(labelBuf), "%s (%d)",
                              bankName.buffer(), (int)presets.size());
                
                if (ImGui::BeginMenu(labelBuf)) {
                    if (presets.empty()) {
                        ImGui::MenuItem("(No presets)", nullptr, false, false);
                    } else {
                        for (size_t i = 0; i < presets.size(); i++) {
                            char presetLabel[256];
                            std::snprintf(presetLabel, sizeof(presetLabel), "%s##%s%d",
                                          presets[i].buffer(), bankName.buffer(), (int)i);
                            
                            if (ImGui::MenuItem(presetLabel)) {
                                if (ui->fPresetManager->loadPresetFromBank(
                                        bankName.buffer(), presets[i].buffer()))
                                {
                                    ui->_updateState(presets[i].buffer(),
                                                     bankName.buffer(), false);
                                }
                            }
                        }
                    }

                    ImGui::Separator();

                    // Save current preset to this bank
                    if (ImGui::MenuItem("Save Current Preset Here...")) {
                        strncpy(_presetNameEditorBuffer, ui->fCurrentPresetName.buffer(),
                                MAX_PRESET_NAME_LENGTH);
                        _presetNameEditorBuffer[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                        _pendingSaveBankName = bankName.buffer();
                        requestSavePresetPopup = true;
                    }

                    // Overwrite current preset in this bank
                    const bool isCurrentInThisBank = (ui->fCurrentPresetBank == bankName);
                    if (ImGui::MenuItem("Overwrite Current", nullptr, false, isCurrentInThisBank))
                    {
                        strncpy(_pendingSavePresetName, ui->fCurrentPresetName.buffer(),
                                MAX_PRESET_NAME_LENGTH);
                        _pendingSavePresetName[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                        _pendingSaveBankName = bankName.buffer();
                        requestConfirmOverwritePresetPopup = true;
                        requestConfirmOverwritePresetType =
                            RequestConfirmOverwriteType::kOverwriteCurrentPreset;
                    }

                    ImGui::Separator();

                    // Export this bank to file
                    if (ImGui::MenuItem("Export Bank...")) {
                        _fileBrowserBankName = bankName.buffer();
                        char defaultFilename[256];
                        std::snprintf(defaultFilename, sizeof(defaultFilename), "%s%s",
                                      bankName.buffer(), USER_PRESET_BANK_EXTENSION);
                        DGL_NAMESPACE::FileBrowserOptions opts;
                        opts.saving = true;
                        opts.title = "Export Bank";
                        opts.defaultName = defaultFilename;
                        if (_fileBrowserHandle != nullptr)
                            DGL_NAMESPACE::fileBrowserClose(_fileBrowserHandle);
                        _fileBrowserHandle = DGL_NAMESPACE::fileBrowserCreate(
                            false, getWindow().getNativeWindowHandle(),
                            getScaleFactor(), opts);
                        _fileBrowserAction = kFileBrowserExportBank;
                    }

                    // Rename this bank
                    if (ImGui::MenuItem("Rename Bank...")) {
                        strncpy(_bankNameEditorBuffer, bankName.buffer(), MAX_PRESET_NAME_LENGTH);
                        _bankNameEditorBuffer[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                        _fileBrowserBankName = bankName.buffer();
                        requestRenameBankPopup = true;
                    }

                    // Delete this bank
                    if (ImGui::MenuItem("Delete Bank...")) {
                        _fileBrowserBankName = bankName.buffer();
                        requestDeleteBankPopup = true;
                    }
                    
                    ImGui::EndMenu();
                }
            }


            // Bank Management Section
            ImGui::Separator();
            
            if (ImGui::MenuItem("New Bank...")) {
                memset(_bankNameEditorBuffer, '\0', MAX_PRESET_NAME_LENGTH);
                requestNewBankPopup = true;
            }

            if (ImGui::MenuItem("Import Bank...")) {
                DGL_NAMESPACE::FileBrowserOptions opts;
                opts.saving = false;
                opts.title = "Import Preset Bank";
                if (_fileBrowserHandle != nullptr)
                    DGL_NAMESPACE::fileBrowserClose(_fileBrowserHandle);
                _fileBrowserHandle = DGL_NAMESPACE::fileBrowserCreate(
                    false, getWindow().getNativeWindowHandle(), getScaleFactor(), opts);
                _fileBrowserAction = kFileBrowserImportBank;
            }
        }

        const String sectionLabel = String("Preset: ") + ui->fCurrentPresetName
                                    + String(ui->fPresetIsModified ? "*" : "");
        ImGui::SeparatorText(sectionLabel.buffer());
        {
            // Save to Default Bank (quick save)
            if (ImGui::MenuItem("Save to Default Bank"))
            {
                // Check if preset with same name already exists
                bool exists = false;
                for (size_t i = 0; i < ui->fPresetManager->getDefaultBankPresetCount(); i++) {
                    if (ui->fPresetManager->getDefaultBankPresetName(i) == ui->fCurrentPresetName) {
                        exists = true;
                        break;
                    }
                }

                if (exists) {
                    strncpy(_pendingSavePresetName, ui->fCurrentPresetName.buffer(),
                            MAX_PRESET_NAME_LENGTH);
                    _pendingSavePresetName[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                    _pendingSaveBankName = DEFAULT_USER_BANK_NAME;
                    requestConfirmOverwritePresetPopup = true;
                    requestConfirmOverwritePresetType =
                        RequestConfirmOverwriteType::kOverwriteInDefaultBank;
                } else {
                    SynthProgram snapshot = ui->fPresetManager->captureCurrentParameters();
                    const char* presetName = ui->fCurrentPresetName.buffer();
                    ui->fPresetManager->savePresetToDefaultBank(presetName, snapshot);
                    ui->_updateState(presetName, DEFAULT_USER_BANK_NAME, false);
                }
            }
            
            if (ImGui::MenuItem("Save As..."))
            {
                strncpy(_presetNameEditorBuffer, ui->fCurrentPresetName.buffer(),
                        MAX_PRESET_NAME_LENGTH);
                _presetNameEditorBuffer[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                _pendingSaveBankName = DEFAULT_USER_BANK_NAME;
                requestSavePresetPopup = true;
            }

            ImGui::Separator();
            
            // Check if current preset is from a user-editable bank (not factory or single imported)
            const bool isUserPreset =
                (ui->fCurrentPresetBank != BANK_NAME_FOR_SINGLE_IMPORTED_PRESET) &&
                (ui->fCurrentPresetBank != FACTORY_BANK_NAME);
            
            if (ImGui::MenuItem("Rename Preset...", nullptr, false, isUserPreset))
            {
                strncpy(_presetNameEditorBuffer, ui->fCurrentPresetName.buffer(),
                        MAX_PRESET_NAME_LENGTH);
                _presetNameEditorBuffer[MAX_PRESET_NAME_LENGTH - 1] = '\0';
                requestRenamePresetPopup = true;
            }
            
            if (ImGui::MenuItem("Delete Preset...", nullptr, false, isUserPreset))
            {
                requestDeletePresetPopup = true;
            }
            
            ImGui::Separator();
            
            // Import/Export
            if (ImGui::MenuItem("Export Preset to File..."))
            {
                String defaultPresetFilename =
                    ui->fCurrentPresetName + String(USER_PRESET_FILE_EXTENSION);
                DGL_NAMESPACE::FileBrowserOptions opts;
                opts.saving = true;
                opts.title = "Export Preset";
                opts.defaultName = defaultPresetFilename.buffer();
                if (_fileBrowserHandle != nullptr)
                    DGL_NAMESPACE::fileBrowserClose(_fileBrowserHandle);
                _fileBrowserHandle = DGL_NAMESPACE::fileBrowserCreate(
                    false, getWindow().getNativeWindowHandle(), getScaleFactor(), opts);
                _fileBrowserAction = kFileBrowserExportPreset;
            }
            
            if (ImGui::MenuItem("Import Preset from File..."))
            {
                DGL_NAMESPACE::FileBrowserOptions opts;
                opts.saving = false;
                opts.title = "Import Preset";
                if (_fileBrowserHandle != nullptr)
                    DGL_NAMESPACE::fileBrowserClose(_fileBrowserHandle);
                _fileBrowserHandle = DGL_NAMESPACE::fileBrowserCreate(
                    false, getWindow().getNativeWindowHandle(), getScaleFactor(), opts);
                _fileBrowserAction = kFileBrowserImportPreset;
            }
        }

        ImGui::EndPopup();
    }
}

void ImGuiUI::_buildPresetManagementPopups()
{
    // Always center this window when appearing
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Save Preset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Bank selector combo
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Target bank:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::BeginCombo("##SaveBankSelect", _pendingSaveBankName.c_str()))
        {
            // Default user bank
            const bool isSelected = (_pendingSaveBankName == DEFAULT_USER_BANK_NAME);
            if (ImGui::Selectable(DEFAULT_USER_BANK_NAME, isSelected))
                _pendingSaveBankName = DEFAULT_USER_BANK_NAME;
            if (isSelected) ImGui::SetItemDefaultFocus();
            // Imported banks
            for (const auto& bk : importedBanks)
            {
                const bool isBkSelected = (_pendingSaveBankName == bk.buffer());
                if (ImGui::Selectable(bk.buffer(), isBkSelected))
                    _pendingSaveBankName = bk.buffer();
                if (isBkSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Dummy(ImVec2(0, 2));

        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputText("##PresetNameSaveInput", _presetNameEditorBuffer, MAX_PRESET_NAME_LENGTH);

        // Check if preset already exists in the target bank
        bool presetAlreadyExists = false;
        if (_pendingSaveBankName == DEFAULT_USER_BANK_NAME) {
            for (size_t i = 0; i < ui->fPresetManager->getDefaultBankPresetCount(); i++) {
                if (ui->fPresetManager->getDefaultBankPresetName(i) == String(_presetNameEditorBuffer)) {
                    presetAlreadyExists = true;
                    break;
                }
            }
        } else {
            auto it = importedBankPresets.find(_pendingSaveBankName);
            if (it != importedBankPresets.end()) {
                for (const auto& p : it->second) {
                    if (p == String(_presetNameEditorBuffer)) {
                        presetAlreadyExists = true;
                        break;
                    }
                }
            }
        }

        if (presetAlreadyExists) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.0f, 1.0f));
            ImGui::TextWrapped("A preset with this name already exists. Saving will overwrite it.");
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();

        const bool presetNameIsEmpty = (_presetNameEditorBuffer[0] == '\0');
        if (presetNameIsEmpty)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("Preset name cannot be empty.");
            ImGui::PopStyleColor();
        }

        const char* saveButtonLabel = presetAlreadyExists ? "Overwrite" : "Save";
        ImGui::BeginDisabled(presetNameIsEmpty);
        if (ImGui::Button(saveButtonLabel, ImVec2(120, 0)))
        {
            SynthProgram snapshot = ui->fPresetManager->captureCurrentParameters();
            
            if (_pendingSaveBankName == DEFAULT_USER_BANK_NAME) {
                ui->fPresetManager->savePresetToDefaultBank(_presetNameEditorBuffer, snapshot);
                ui->_updateState(_presetNameEditorBuffer, DEFAULT_USER_BANK_NAME, false);
            } else {
                ui->fPresetManager->savePresetToBank(_pendingSaveBankName.c_str(),
                                                      _presetNameEditorBuffer, snapshot);
                ui->_updateState(_presetNameEditorBuffer, _pendingSaveBankName.c_str(), false);
                _shouldRefreshBankList = true;
            }

            ImGui::CloseCurrentPopup();
            requestSavePresetPopup = false;
        }
        ImGui::EndDisabled();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            requestSavePresetPopup = false;
        }

        ImGui::EndPopup();
    }

    // Always center this window when appearing
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Rename Preset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Only single imported presets cannot be renamed
        const bool isNonRenameablePreset =
            (ui->fCurrentPresetBank == BANK_NAME_FOR_SINGLE_IMPORTED_PRESET);

        if (isNonRenameablePreset)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
            ImGui::TextWrapped(
                "This preset was imported from a file and is not part of a bank.");
            ImGui::TextWrapped("Use 'Save As...' to save it to a bank with a new name.");
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0, 8));

            if (ImGui::Button("OK", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
                requestRenamePresetPopup = false;
            }
        }
        else
        {
            // User preset (Default Bank or imported bank) - allow renaming
            ImGui::Text("Rename preset in '%s':", ui->fCurrentPresetBank.buffer());
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::SetNextItemWidth(250.0f);
            ImGui::InputText("##PresetNameInput", _presetNameEditorBuffer, MAX_PRESET_NAME_LENGTH);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Separator();

            const bool renamePresetDisabled =
                (_presetNameEditorBuffer[0] == '\0') ||
                (ui->fCurrentPresetName == String(_presetNameEditorBuffer));
            if (renamePresetDisabled)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::TextWrapped(_presetNameEditorBuffer[0] == '\0'
                    ? "Preset name cannot be empty."
                    : "New name is the same as the current name.");
                ImGui::PopStyleColor();
            }

            ImGui::BeginDisabled(renamePresetDisabled);
            if (ImGui::Button("Rename", ImVec2(120, 0)))
            {
                String oldName = ui->fCurrentPresetName;
                String newName = String(_presetNameEditorBuffer);
                String bankName = ui->fCurrentPresetBank;

                if (ui->fPresetManager->renamePresetInBank(bankName.buffer(),
                                                            oldName.buffer(),
                                                            newName.buffer()))
                {
                    ui->_updateState(newName.buffer(), bankName.buffer(), false);
                    _shouldRefreshBankList = true;
                    ui->logAndShowMessage("Successfully renamed preset from '%s' to '%s'",
                                          oldName.buffer(), newName.buffer());
                }
                else
                {
                    ui->logAndShowMessage("Failed to rename preset from '%s' to '%s'",
                                          oldName.buffer(), newName.buffer());
                }

                ImGui::CloseCurrentPopup();
                requestRenamePresetPopup = false;
            }
            ImGui::EndDisabled();
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                requestRenamePresetPopup = false;
            }
        }

        ImGui::EndPopup();
    }
    
    // Always center this window when appearing
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Delete Preset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Delete preset from '%s'", ui->fCurrentPresetBank.buffer());
        ImGui::Dummy(ImVec2(0, 8));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("Are you sure you want to delete preset '%s'?",
                            ui->fCurrentPresetName.buffer());
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::TextWrapped("This action cannot be undone.");
        ImGui::PopStyleColor();

        ImGui::Separator();

        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            String presetName = ui->fCurrentPresetName;
            String bankName = ui->fCurrentPresetBank;

            if (ui->fPresetManager->deletePresetFromBank(bankName.buffer(), presetName.buffer()))
            {
                ui->logAndShowMessage("Successfully deleted preset '%s' from bank '%s'",
                                       presetName.buffer(), bankName.buffer());
                _shouldRefreshBankList = true;

                // Reset to factory default preset
                ui->fPresetManager->loadDefaultProgram();
                ui->_updateState(DEFAULT_PRESET_NAME, FACTORY_BANK_NAME, false);
            }
            else
            {
                ui->logAndShowMessage("Failed to delete preset '%s' from bank '%s'",
                                       presetName.buffer(), bankName.buffer());
            }

            ImGui::CloseCurrentPopup();
            requestDeletePresetPopup = false;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            requestDeletePresetPopup = false;
        }

        ImGui::EndPopup();
    }

    // Always center this window when appearing
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Overwrite Preset?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.0f, 1.0f));
        const char* dialogText =
            (requestConfirmOverwritePresetType ==
             RequestConfirmOverwriteType::kOverwriteInDefaultBank)
                ? "Preset '%s' already exists in '%s'."
                : "About to overwrite the currently loaded preset '%s' in bank '%s'.";
        ImGui::TextWrapped(dialogText, _pendingSavePresetName, _pendingSaveBankName.c_str());
        ImGui::TextWrapped("Do you want to overwrite it?");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::Separator();

        if (ImGui::Button("Overwrite", ImVec2(120, 0)))
        {
            SynthProgram snapshot = ui->fPresetManager->captureCurrentParameters();

            if (_pendingSaveBankName == DEFAULT_USER_BANK_NAME) {
                ui->fPresetManager->savePresetToDefaultBank(_pendingSavePresetName, snapshot);
                ui->_updateState(_pendingSavePresetName, DEFAULT_USER_BANK_NAME, false);
            } else {
                ui->fPresetManager->savePresetToBank(_pendingSaveBankName.c_str(),
                                                      _pendingSavePresetName, snapshot);
                ui->_updateState(_pendingSavePresetName, _pendingSaveBankName.c_str(), false);
                _shouldRefreshBankList = true;
            }

            ImGui::CloseCurrentPopup();
            requestConfirmOverwritePresetPopup = false;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            requestConfirmOverwritePresetPopup = false;
        }

        ImGui::EndPopup();
    }

    // Always center this window when appearing
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("New Bank", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create a new preset bank:");
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputText("##NewBankNameInput", _bankNameEditorBuffer, MAX_PRESET_NAME_LENGTH);

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();

        const bool bankNameIsEmpty = (_bankNameEditorBuffer[0] == '\0');
        ImGui::BeginDisabled(bankNameIsEmpty);
        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            String newBankName(_bankNameEditorBuffer);
            if (ui->fPresetManager->createNewBank(newBankName.buffer()))
            {
                ui->logAndShowMessage("Bank '%s' created successfully.", newBankName.buffer());
                _shouldRefreshBankList = true;
            }
            else
            {
                ui->logAndShowMessage("Failed to create bank '%s'. It may already exist.",
                                       newBankName.buffer());
            }
            ImGui::CloseCurrentPopup();
            requestNewBankPopup = false;
        }
        ImGui::EndDisabled();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            requestNewBankPopup = false;
        }

        ImGui::EndPopup();
    }

    // Always center this window when appearing
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Rename Bank", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        char renameBankInfoBuf[256];
        std::snprintf(renameBankInfoBuf, sizeof(renameBankInfoBuf),
                      "Rename bank '%s':", _fileBrowserBankName.c_str());
        ImGui::Text("%s", renameBankInfoBuf);
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputText("##RenameBankInput", _bankNameEditorBuffer, MAX_PRESET_NAME_LENGTH);

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();

        const bool renameBankDisabled =
            (_bankNameEditorBuffer[0] == '\0') ||
            (std::strcmp(_bankNameEditorBuffer, _fileBrowserBankName.c_str()) == 0);
        if (renameBankDisabled)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped(_bankNameEditorBuffer[0] == '\0'
                ? "Bank name cannot be empty."
                : "New name is the same as the current name.");
            ImGui::PopStyleColor();
        }

        ImGui::BeginDisabled(renameBankDisabled);
        if (ImGui::Button("Rename", ImVec2(120, 0)))
        {
            String oldName(_fileBrowserBankName.c_str());
            String newName(_bankNameEditorBuffer);

            if (ui->fPresetManager->renameBankByName(oldName.buffer(), newName.buffer()))
            {
                // If current preset was from the renamed bank, update the bank reference
                if (ui->fCurrentPresetBank == oldName)
                    ui->_updateState(ui->fCurrentPresetName.buffer(), newName.buffer(),
                                     ui->fPresetIsModified);

                ui->logAndShowMessage("Bank renamed from '%s' to '%s'.",
                                       oldName.buffer(), newName.buffer());
                _shouldRefreshBankList = true;
            }
            else
            {
                ui->logAndShowMessage(
                    "Failed to rename bank '%s'. Target name may already exist.",
                    oldName.buffer());
            }
            ImGui::CloseCurrentPopup();
            requestRenameBankPopup = false;
        }
        ImGui::EndDisabled();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            requestRenameBankPopup = false;
        }

        ImGui::EndPopup();
    }

    // Always center this window when appearing
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Delete Bank", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        char deleteBankInfoBuf[256];
        std::snprintf(deleteBankInfoBuf, sizeof(deleteBankInfoBuf),
                      "Are you sure you want to delete bank '%s'?",
                      _fileBrowserBankName.c_str());
        ImGui::TextWrapped("%s", deleteBankInfoBuf);
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::TextWrapped(
            "All presets in this bank will be deleted. This action cannot be undone.");
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();

        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            String bankToDelete(_fileBrowserBankName.c_str());

            if (ui->fPresetManager->deleteBankByName(bankToDelete.buffer()))
            {
                // If current preset was from the deleted bank, reset to default
                if (ui->fCurrentPresetBank == bankToDelete)
                {
                    ui->fPresetManager->loadDefaultProgram();
                    ui->_updateState(DEFAULT_PRESET_NAME, FACTORY_BANK_NAME, false);
                }
                ui->logAndShowMessage("Bank '%s' deleted successfully.", bankToDelete.buffer());
                _shouldRefreshBankList = true;
            }
            else
            {
                ui->logAndShowMessage("Failed to delete bank '%s'.", bankToDelete.buffer());
            }

            ImGui::CloseCurrentPopup();
            requestDeleteBankPopup = false;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            requestDeleteBankPopup = false;
        }

        ImGui::EndPopup();
    }
}
