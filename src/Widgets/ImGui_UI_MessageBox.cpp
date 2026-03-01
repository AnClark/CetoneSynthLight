#include "ImGui_UI.hpp"

/**
 * @brief Handle message box
 *
 * This function implements message box feature in CetoneSynthLight.
 * It polls message queue. If there's a message in queue, it shows a message box
 * on the center of the plugin UI immediately.
 * It must be called every frame from onImGuiDisplay() to poll the message queue
 * and show message box popup.
 *
 * ## How to show message
 *
 * - Directly push a std::string to ImGui::messageBoxQueue.
 * - Invoke ui->logAndShowMessage(). This prints log on console as well.
 *
 * @see CCetoneUI::logAndShowMessage()
 */
void ImGuiUI::_handleMessageBoxIdle()
{
    if (!messageBoxQueue.empty())
    {
        _requestMessagePopup = true;
    }

    if (_requestMessagePopup)
    {
        ImGui::OpenPopup("Message");
        _requestMessagePopup = false;
    }

    // Always center this window when appearing
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Message", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Text("%s", messageBoxQueue.front().c_str());
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            // Apply a mutex to avoid possible conflict
            std::lock_guard<std::mutex> lock(_messageQueueMutex);

            messageBoxQueue.pop();

            ImGui::CloseCurrentPopup();
            _requestMessagePopup = false;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}
