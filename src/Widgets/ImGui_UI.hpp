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

// Forward decls.
class CCetoneUI;


// --------------------------------------------------------------------------------------------------------------------

class ImGuiUI : public ImGuiTopLevelWidget
{

public:
    CCetoneUI *ui;

    bool isAboutWindowOpen = false;
    uint16_t requestMenuId = 0;
    
    ImVec2 menuPos{0, 0};

    double userScaling = 1.0f;

    ImGuiUI(TopLevelWidget* const tlw, CCetoneUI* const ui) : 
        ImGuiTopLevelWidget(tlw->getWindow()),
        ui(ui)
        {
        }

protected:
    void onImGuiDisplay() override;

private:
    void _triggerParamUpdate(uint32_t paramId, float newValue);

    uint16_t _requestedModParam = 0;
};
