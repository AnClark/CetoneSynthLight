/*
 * Inspector Window for DPF
 * Copyright (C) 2022-2025 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include "DearImGui.hpp"
#include "FileBrowserDialog.hpp"

#include <map>
#include <queue>
#include <string>
#include <mutex>
#include "extra/String.hpp"

// Forward decls.
class CCetoneUI;

// Constants.
constexpr auto MAX_PRESET_NAME_LENGTH = 128;

// --------------------------------------------------------------------------------------------------------------------

class ImGuiUI : public ImGuiTopLevelWidget {
    CCetoneUI* ui; // UI instance pointer
    double     userScaling = 1.0f; // User scaling factor for UI elements

public:
    // ----------------------------------------------------------------
    // Window states

    bool isAboutWindowOpen = false; // "About" window visibility flag

    // ----------------------------------------------------------------
    // Parameter menu stuff

    // Set this variable to the parameter ID for which the parameter menu should be opened.
    // The menu will open on the next UI update and then reset this variable to 0.
    uint16_t requestMenuId = 0;

    // Position of the parameter menu when opened.
    ImVec2 menuPos { 0, 0 };

    // ----------------------------------------------------------------
    // Message box stuff

    // Message box queue.
    // Pushing a string to this queue will trigger a message box popup.
    std::queue<std::string> messageBoxQueue;

protected:
    // ----------------------------------------------------------------
    // Preset Manager stuff

    // Preset management popup flags
    bool requestRenamePresetPopup = false;
    bool requestSavePresetPopup = false;
    bool requestDeletePresetPopup = false;

    // Bank management popup flags
    bool requestNewBankPopup = false;
    bool requestRenameBankPopup = false;
    bool requestDeleteBankPopup = false;

    // Overwrite confirmation popup flag
    enum class RequestConfirmOverwriteType {
        kOverwriteInDefaultBank = 2 << 2,   // Overwrite a preset in the Default Bank (quick save)
        kOverwriteCurrentPreset,
    };
    bool requestConfirmOverwritePresetPopup = false;
    RequestConfirmOverwriteType requestConfirmOverwritePresetType;

    // ----------------------------------------------------------------
    // File browser stuff (works together with Preset Manager)

    enum FileBrowserAction {
        kFileBrowserNone,
        kFileBrowserExportPreset,
        kFileBrowserImportPreset,
        kFileBrowserExportBank,
        kFileBrowserImportBank,
    };

public:
    ImGuiUI(TopLevelWidget* const tlw, CCetoneUI* const ui)
        : ImGuiTopLevelWidget(tlw->getWindow())
        , ui(ui)
    {
        memset(_presetNameEditorBuffer, '\0', sizeof(char) * MAX_PRESET_NAME_LENGTH);
        memset(_bankNameEditorBuffer, '\0', sizeof(char) * MAX_PRESET_NAME_LENGTH);
        memset(_pendingSavePresetName, '\0', sizeof(char) * MAX_PRESET_NAME_LENGTH);
    }

protected:
    // ----------------------------------------------------------------
    // Widget callbacks

    void onImGuiDisplay() override;

private:
    // -------------------------------------------------------------------
    // Utility functions

    void _triggerParamUpdate(uint32_t paramId, float newValue);

    // -------------------------------------------------------------------
    // Parameter menu stuff

    uint16_t _requestedModParam = 0;

    // -------------------------------------------------------------------
    // Message box stuff

    bool       _requestMessagePopup = false;
    void       _handleMessageBoxIdle();
    std::mutex _messageQueueMutex;

    // -------------------------------------------------------------------
    // Preset Manager Stuff

    void _handlePresetModalPopupRequests();
    void _buildPresetManagementMenu();
    void _buildPresetManagementPopups();

    // Which bank to operate on in file browser / bank management popups
    std::string _fileBrowserBankName;

    // ImGui::TextInput buffers for preset name editing
    char _presetNameEditorBuffer[MAX_PRESET_NAME_LENGTH];
    char _bankNameEditorBuffer[MAX_PRESET_NAME_LENGTH];

    // Local cache of bank list for menu display
    std::vector<String>                        importedBanks;
    std::map<std::string, std::vector<String>> importedBankPresets;
    bool                                       _shouldRefreshBankList = true;

    // Pending save data for overwrite confirmation
    char        _pendingSavePresetName[MAX_PRESET_NAME_LENGTH];
    std::string _pendingSaveBankName;

    // -------------------------------------------------------------------
    // File browser stuff (works together with Preset Manager)

    DGL_NAMESPACE::FileBrowserHandle _fileBrowserHandle = nullptr;
    FileBrowserAction                _fileBrowserAction = kFileBrowserNone;
    void                             _handleFileBrowserIdle();
};
