#include "TestImGuiUI.hpp"

void TestImGuiUI::onImGuiDisplay()
{
    double scaleFactor = getScaleFactor() * userScaling;
    const double initialSize = 800 * scaleFactor;

    ImGui::SetNextWindowPos(ImVec2(initialSize / 4, initialSize / 16), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(initialSize / 2, initialSize / 3), ImGuiCond_Once);

    if (isTestWindowOpen)
    {
        ImGui::Begin("Dear ImGui Popup Window", &isTestWindowOpen, ImGuiWindowFlags_NoCollapse);
        {
            ImGui::Text("This is a test window!");
        }
        ImGui::End(); 
    }


    // Specify menu position
    ImGui::SetNextWindowPos(menuPos); // 指定屏幕坐标位置

    // Here, variable `requestTestMenuOpen` acts as an "event flag" to request ImGui to show the menu.
    //
    // Dear ImGui has its own mechanism to show popup menus, which does not require a flag to control its exisitance.
    // This is quite different from window (ImGui::Begin()).
    // So just call this function once, your popup will stick on the screen unless you do some operations.
    //
    if (requestTestMenuOpen)
    {
        ImGui::OpenPopup("my_context_menu");
        requestTestMenuOpen = false;        // Reset flag state
    }

    // Create popup menu
    // [NOTICE] This call MUST be put after ImGui::OpenPopup(), otherwise popup won't show!
    if (ImGui::BeginPopup("my_context_menu"))
    {
        // BUG: This menu item does not work as expected behavior!
        if (ImGui::MenuItem("Toggle Test Window", NULL, &isTestWindowOpen))
        {
            d_stderr("Selected menu item: 'Toggle Test Window'");
            this->isTestWindowOpen = !this->isTestWindowOpen;
        }

        if (ImGui::MenuItem(isTestWindowOpen ? "Test Window: Opened" : "Test Window: Hidden"))
        {
            this->isTestWindowOpen = !this->isTestWindowOpen;
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Option 1")) { ImGui::CloseCurrentPopup(); }
        if (ImGui::MenuItem("Option 2")) { ImGui::CloseCurrentPopup(); }
        if (ImGui::BeginMenu("Submenu")) {
            ImGui::MenuItem("Sub-option 1");
            ImGui::MenuItem("Sub-option 2");
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}