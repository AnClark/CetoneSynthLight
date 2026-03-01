#include "CetoneUI.hpp"  // For class CCetoneUI
#include "ImGui_UI.hpp"
#include "Defines.h"         // For USER_PRESET_FILE_EXTENSION / USER_PRESET_BANK_EXTENSION
#include "extra/String.hpp"  // For DPF String class


/**
 * @brief Handle file browser dialog in idle state (polling-based approach)
 *
 * This function implements asynchronous file dialog handling using DPF's
 * low-level FileBrowserDialog API. It must be called every frame from
 * onImGuiDisplay() to poll the dialog state and process user selections.
 *
 * ### Polling Mechanism
 * Unlike callback-based approaches, this uses manual polling:
 * - fileBrowserCreate() spawns native OS file dialog (non-blocking)
 * - fileBrowserIdle() returns true when user closes dialog (either confirmed or cancelled)
 * - fileBrowserGetPath() retrieves selected path (nullptr if cancelled)
 * - fileBrowserClose() cleanup resources
 *
 * ### File Operations
 * - **Export**: Captures current preset, auto-appends .clight extension, writes JSON
 * - **Import**: Reads .clight JSON file, deserializes, loads to synth, marks as
 *   BANK_NAME_FOR_SINGLE_IMPORTED_PRESET
 *
 * @see DGL_NAMESPACE::fileBrowserCreate() - Creates file dialog
 * @see DGL_NAMESPACE::fileBrowserIdle() - Polls dialog completion
 * @see DGL_NAMESPACE::fileBrowserGetPath() - Gets selected file path
 * @see DGL_NAMESPACE::fileBrowserClose() - Cleanup dialog resources
 * @see PresetManager::exportCurrentPresetToFile() - Export implementation
 * @see PresetManager::importPresetFromFile() - Import implementation
 *
 * @note Must be called every frame to maintain dialog responsiveness
 * @note Dialog handle is stored in _fileBrowserHandle (nullptr when inactive)
 * @note Action type is stored in _fileBrowserAction
 */
void ImGuiUI::_handleFileBrowserIdle() {
    if (_fileBrowserHandle == nullptr) return;

    // Check if file browser is done
    if (DGL_NAMESPACE::fileBrowserIdle(_fileBrowserHandle)) {
        // Get the selected path
        const char* selectedPath =
            DGL_NAMESPACE::fileBrowserGetPath(_fileBrowserHandle);

        if (selectedPath != nullptr && selectedPath[0] != '\0') {
            // Process the selected file based on action
            switch (_fileBrowserAction) {
                case kFileBrowserExportPreset: {
                    // Ensure file has correct extension
                    String filePath(selectedPath);
                    if (!filePath.endsWith(USER_PRESET_FILE_EXTENSION))
                        filePath = filePath + String(USER_PRESET_FILE_EXTENSION);

                    if (ui->fPresetManager->exportCurrentPresetToFile(filePath.buffer())) {
                        ui->logAndShowMessage("Preset '%s' exported successfully.",
                                              ui->fCurrentPresetName.buffer());
                    } else {
                        ui->logAndShowMessage("Failed to export preset to file.");
                    }
                    break;
                }

                case kFileBrowserImportPreset: {
                    String importedPresetName;
                    if (ui->fPresetManager->importPresetFromFile(selectedPath, &importedPresetName)) {
                        ui->_updateState(importedPresetName.buffer(),
                                         BANK_NAME_FOR_SINGLE_IMPORTED_PRESET, false);
                        ui->logAndShowMessage(
                            "Preset '%s' imported. Use 'Save As...' to keep it.",
                            importedPresetName.buffer());
                    } else {
                        ui->logAndShowMessage("Failed to import preset from file.");
                    }
                    break;
                }

                case kFileBrowserExportBank: {
                    // Export the bank specified by _fileBrowserBankName to file
                    String filePath(selectedPath);
                    if (!filePath.endsWith(USER_PRESET_BANK_EXTENSION))
                        filePath = filePath + String(USER_PRESET_BANK_EXTENSION);

                    if (ui->fPresetManager->exportBankToFile(_fileBrowserBankName.c_str(),
                                                             filePath.buffer())) {
                        ui->logAndShowMessage("Bank '%s' exported successfully.",
                                              _fileBrowserBankName.c_str());
                    } else {
                        ui->logAndShowMessage("Failed to export bank '%s'.",
                                              _fileBrowserBankName.c_str());
                    }
                    break;
                }

                case kFileBrowserImportBank: {
                    if (ui->fPresetManager->importBankFromFile(selectedPath)) {
                        _shouldRefreshBankList = true;
                        ui->logAndShowMessage("Bank imported successfully.");
                    } else {
                        ui->logAndShowMessage("Failed to import bank from file.");
                    }
                    break;
                }

                default:
                    break;
            }
        }

        // Close and cleanup
        DGL_NAMESPACE::fileBrowserClose(_fileBrowserHandle);
        _fileBrowserHandle = nullptr;
        _fileBrowserAction = kFileBrowserNone;
    }
}
