#include "ImGui_UI.hpp"
#include "DistrhoPluginInfo.h"

#include "Structures.h"
#include "Defines.h"

#include "CetoneUI.hpp" // For class CCetoneUI

void ImGuiUI::onImGuiDisplay()
{
    double scaleFactor = getScaleFactor() * userScaling;
    const double initialSize = 800 * scaleFactor;

    //
    // "About" Window
    //
    {
        ImGui::SetNextWindowPos(ImVec2(initialSize / 4, initialSize / 16), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(600, 250), ImGuiCond_Once);

        if (isAboutWindowOpen)
        {
            ImGui::Begin("About " DISTRHO_PLUGIN_NAME, &isAboutWindowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            {
                ImGui::SeparatorText("Cetone Synth Light");
                ImGui::Text("Light-weight monophonic analogue-style synthesizer, by Neotec Software.");
                ImGui::Text("Copyright © 2007, Neotec Software.");
                ImGui::Text("Copyright © 2024-2025, AnClark Liu <clarklaw4701@qq.com>.");

                ImGui::SeparatorText("Authors");
                ImGui::BulletText("René 'Neotec' Jeschke - Original developer");
                ImGui::BulletText("AnClark Liu - Ported to DPF, Further developments");

                ImGui::SeparatorText("License");
                ImGui::BulletText("This project is licensed under GNU General Public License, version 3.");

                ImGui::Text("\n");
                ImGui::Dummy(ImVec2(490, 0));
                ImGui::SameLine();
                if (ImGui::Button("OK", ImVec2(80, 0)))
                    isAboutWindowOpen = false;
            }
            ImGui::End(); 
        }
    }

    //
    // Handle menu opening requests
    //
    // Here, variable `requestTestMenuOpen` acts as an "event flag" to request ImGui to show the menu.
    //
    // Dear ImGui has its own mechanism to show popup menus, which does not require a flag to control its exisitance.
    // This is quite different from window (ImGui::Begin()).
    // So just call this function once, your popup will stick on the screen unless you do some operations.
    //
    switch (requestMenuId)
    {
        case pOsc1Wave:
            ImGui::OpenPopup("menu_osc1_wave");
            requestMenuId = 0;
            break;
        case pOsc2Wave:
            ImGui::OpenPopup("menu_osc2_wave");
            requestMenuId = 0;
            break;
        case pOsc3Wave:
            ImGui::OpenPopup("menu_osc3_wave");
            requestMenuId = 0;
            break;
        case pFilterType:
            ImGui::OpenPopup("menu_filter_type");
            requestMenuId = 0;
            break;
        case pFilterMode:
            ImGui::OpenPopup("menu_filter_mode");
            requestMenuId = 0;
            break;
        case pLfo1Wave:
            ImGui::OpenPopup("menu_lfo1_wave");
            requestMenuId = 0;
            break;
        case pArpMode:
            ImGui::OpenPopup("menu_arp_mode");
            requestMenuId = 0;
            break;

        case pMod1Src:
        case pMod2Src:
        case pMod3Src:
        case pMod4Src:
            ImGui::OpenPopup("menu_mod_src");
            _requestedModParam = requestMenuId;
            requestMenuId = 0;
            break;
        
        case pMod1Dest:
        case pMod2Dest:
        case pMod3Dest:
        case pMod4Dest:
            ImGui::OpenPopup("menu_mod_dest");
            _requestedModParam = requestMenuId;
            requestMenuId = 0;
            break;

        default:
            requestMenuId = 0;
    }

    //
    // Create popup menus
    // [NOTICE] ImGui::BeginPopup() MUST be put after ImGui::OpenPopup(), otherwise popup won't show!
    //

    ImGui::SetNextWindowPos(menuPos);       // Specify menu position

    if (ImGui::BeginPopup("menu_osc1_wave"))
    {
        ImGui::SeparatorText("Waveform");
        if (ImGui::MenuItem("Saw")) { _triggerParamUpdate(pOsc1Wave, ui->_pi2f(OWAVE_SAW, OWAVE_MAX)); }
        if (ImGui::MenuItem("Pulse")) { _triggerParamUpdate(pOsc1Wave, ui->_pi2f(OWAVE_PULSE, OWAVE_MAX)); }
        if (ImGui::MenuItem("Triangle")) { _triggerParamUpdate(pOsc1Wave, ui->_pi2f(OWAVE_TRI, OWAVE_MAX)); }
        if (ImGui::MenuItem("Sine")) { _triggerParamUpdate(pOsc1Wave, ui->_pi2f(OWAVE_SINE, OWAVE_MAX)); }
        if (ImGui::MenuItem("C64 Noise")) { _triggerParamUpdate(pOsc1Wave, ui->_pi2f(OWAVE_C64NOISE, OWAVE_MAX)); }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(menuPos);       // Specify menu position

    if (ImGui::BeginPopup("menu_osc2_wave"))
    {
        ImGui::SeparatorText("Waveform");
        if (ImGui::MenuItem("Saw")) { _triggerParamUpdate(pOsc2Wave, ui->_pi2f(OWAVE_SAW, OWAVE_MAX)); }
        if (ImGui::MenuItem("Pulse")) { _triggerParamUpdate(pOsc2Wave, ui->_pi2f(OWAVE_PULSE, OWAVE_MAX)); }
        if (ImGui::MenuItem("Triangle")) { _triggerParamUpdate(pOsc2Wave, ui->_pi2f(OWAVE_TRI, OWAVE_MAX)); }
        if (ImGui::MenuItem("Sine")) { _triggerParamUpdate(pOsc2Wave, ui->_pi2f(OWAVE_SINE, OWAVE_MAX)); }
        if (ImGui::MenuItem("C64 Noise")) { _triggerParamUpdate(pOsc2Wave, ui->_pi2f(OWAVE_C64NOISE, OWAVE_MAX)); }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(menuPos);       // Specify menu position

    if (ImGui::BeginPopup("menu_osc3_wave"))
    {
        ImGui::SeparatorText("Waveform");
        if (ImGui::MenuItem("Saw")) { _triggerParamUpdate(pOsc3Wave, ui->_pi2f(OWAVE_SAW, OWAVE_MAX)); }
        if (ImGui::MenuItem("Pulse")) { _triggerParamUpdate(pOsc3Wave, ui->_pi2f(OWAVE_PULSE, OWAVE_MAX)); }
        if (ImGui::MenuItem("Triangle")) { _triggerParamUpdate(pOsc3Wave, ui->_pi2f(OWAVE_TRI, OWAVE_MAX)); }
        if (ImGui::MenuItem("Sine")) { _triggerParamUpdate(pOsc3Wave, ui->_pi2f(OWAVE_SINE, OWAVE_MAX)); }
        if (ImGui::MenuItem("C64 Noise")) { _triggerParamUpdate(pOsc3Wave, ui->_pi2f(OWAVE_C64NOISE, OWAVE_MAX)); }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(menuPos);       // Specify menu position

    if (ImGui::BeginPopup("menu_filter_type"))
    {
        ImGui::SeparatorText("Filter Type");
        if (ImGui::MenuItem("Dirty")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_DIRTY, FTYPE_MAX)); }
        if (ImGui::MenuItem("Moog")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_MOOG, FTYPE_MAX)); }
        if (ImGui::MenuItem("Moog 2")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_MOOG2, FTYPE_MAX)); }
        if (ImGui::MenuItem("Ch12db")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_CH12DB, FTYPE_MAX)); }
        if (ImGui::MenuItem("x0x (303)")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_303, FTYPE_MAX)); }
        if (ImGui::MenuItem("8580")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_8580, FTYPE_MAX)); }
        if (ImGui::MenuItem("Bi12db (Budda)")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_BUDDA, FTYPE_MAX)); }
        ImGui::Separator();
        if (ImGui::MenuItem("No Filter")) { _triggerParamUpdate(pFilterType, ui->_pi2f(FTYPE_NONE, FTYPE_MAX)); }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(menuPos);

    if (ImGui::BeginPopup("menu_filter_mode"))
    {
        bool _isSomeModesUnsupported = false;

        ImGui::SeparatorText("Filter Mode");
        if (ImGui::MenuItem("Low pass")) { _triggerParamUpdate(pFilterMode, ui->_pi2f(FMODE_LOW, FMODE_MAX)); }
        switch (ui->_pf2i(ui->fKnobFilterType->getValue(), FTYPE_MAX))
        {
            case FTYPE_DIRTY:
            case FTYPE_MOOG2:
            case FTYPE_CH12DB:
            case FTYPE_8580:
                break;
            default:
                ImGui::BeginDisabled();
                _isSomeModesUnsupported = true;
        }
        if (ImGui::MenuItem("Band pass")) { _triggerParamUpdate(pFilterMode, ui->_pi2f(FMODE_BAND, FMODE_MAX)); }
        if (ImGui::MenuItem("High pass")) { _triggerParamUpdate(pFilterMode, ui->_pi2f(FMODE_HIGH, FMODE_MAX)); }
        if (ImGui::MenuItem("Notch")) { _triggerParamUpdate(pFilterMode, ui->_pi2f(FMODE_NOTCH, FMODE_MAX)); }

        if (_isSomeModesUnsupported)
            ImGui::EndDisabled();
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(menuPos);       // Specify menu position

    if (ImGui::BeginPopup("menu_lfo1_wave"))
    {
        ImGui::SeparatorText("Waveform");
        if (ImGui::MenuItem("Saw")) { _triggerParamUpdate(pLfo1Wave, ui->_pi2f(OWAVE_SAW, OWAVE_MAX)); }
        if (ImGui::MenuItem("Pulse")) { _triggerParamUpdate(pLfo1Wave, ui->_pi2f(OWAVE_PULSE, OWAVE_MAX)); }
        if (ImGui::MenuItem("Triangle")) { _triggerParamUpdate(pLfo1Wave, ui->_pi2f(OWAVE_TRI, OWAVE_MAX)); }
        if (ImGui::MenuItem("Sine")) { _triggerParamUpdate(pLfo1Wave, ui->_pi2f(OWAVE_SINE, OWAVE_MAX)); }
        if (ImGui::MenuItem("C64 Noise")) { _triggerParamUpdate(pLfo1Wave, ui->_pi2f(OWAVE_C64NOISE, OWAVE_MAX)); }
        ImGui::EndPopup();
    }

    // NOTICE:
    // For menus below, no need to specify menu position. Let Dear ImGui decide menu's position.
    // Otherwise, menu will partially show on the screen due to insufficient space.

    if (ImGui::BeginPopup("menu_arp_mode"))
    {
        ImGui::SeparatorText("Arp Mode");
        if (ImGui::MenuItem("Minor")) { _triggerParamUpdate(pArpMode, ui->_pi2f(0 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("Major")) { _triggerParamUpdate(pArpMode, ui->_pi2f(1 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("Minor + 1 Octave")) { _triggerParamUpdate(pArpMode, ui->_pi2f(2 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("Major + 1 Octave")) { _triggerParamUpdate(pArpMode, ui->_pi2f(3 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("1 Octave")) { _triggerParamUpdate(pArpMode, ui->_pi2f(4 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("2 Octaves")) { _triggerParamUpdate(pArpMode, ui->_pi2f(5 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("Quint")) { _triggerParamUpdate(pArpMode, ui->_pi2f(6 + 1, ARP_MAX + 1)); }
        if (ImGui::MenuItem("Quint 2")) { _triggerParamUpdate(pArpMode, ui->_pi2f(7 + 1, ARP_MAX + 1)); }
        ImGui::Separator();
        if (ImGui::MenuItem("Off")) { _triggerParamUpdate(pArpMode, ui->_pi2f(-1 + 1, ARP_MAX + 1)); }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("menu_mod_src"))
    {
        ImGui::SeparatorText("Modulator Source");
        if (ImGui::MenuItem("Velocity")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_SRC_VEL, MOD_SRC_MAX)); }
        if (ImGui::MenuItem("Controller 1")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_SRC_CTRL1, MOD_SRC_MAX)); }
        if (ImGui::MenuItem("Modulation Envelope")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_SRC_MENV1, MOD_SRC_MAX)); }
        if (ImGui::MenuItem("LFO")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_SRC_LFO1, MOD_SRC_MAX)); }
        if (ImGui::MenuItem("Modulation Env. x LFO")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_SRC_MENV1xLFO1, MOD_SRC_MAX)); }
        ImGui::Separator();
        if (ImGui::MenuItem("Disable")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_SRC_NONE, MOD_SRC_MAX)); }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("menu_mod_dest"))
    {
        ImGui::SeparatorText("Modulator Destination");
        if (ImGui::MenuItem("Main Volume")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_MAINVOL, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Panning")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_PANNING, MOD_DEST_MAX)); }
        ImGui::Separator();
        if (ImGui::MenuItem("Cutoff")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_CUTOFF, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Resonance")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_RESONANCE, MOD_DEST_MAX)); }
        ImGui::Separator();
        if (ImGui::MenuItem("Main Pitch")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_MAINPITCH, MOD_DEST_MAX)); }
        ImGui::Separator();
        if (ImGui::MenuItem("Osc 1 - Volume")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC1VOL, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 2 - Volume")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC2VOL, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 3 - Volume")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC3VOL, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 1 - Pitch")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC1PITCH, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 2 - Pitch")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC2PITCH, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 3 - Pitch")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC3PITCH, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 1 - Pulsewidth")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC1PW, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 2 - Pulsewidth")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC2PW, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Osc 3 - Pulsewidth")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_OSC3PW, MOD_DEST_MAX)); }
        ImGui::Separator();
        if (ImGui::MenuItem("LFO 1 Speed")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_LFO1SPEED, MOD_DEST_MAX)); }
        if (ImGui::MenuItem("Filter Parameter (Env. Mod)")) { _triggerParamUpdate(_requestedModParam, ui->_pi2f(MOD_DEST_ENVMOD, MOD_DEST_MAX)); }

        ImGui::EndPopup();
    }

    //
    // Toolbar area - resides below the plugin logo
    //
    if (ImGui::Begin("Main Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground))
    {
        ImGui::SetWindowPos(ImVec2(0, 60));
        ImGui::SetWindowSize(ImVec2(100, 80));

#ifdef ENABLE_POLYPHONY
        // Polyphony switch
        {
            ImGui::Text("Polyphony");

            String _buttonLabel = String(ui->fMaxPolyphony) + "##PolyphonyButton";
            if (ImGui::Button(_buttonLabel.buffer(), ImVec2(60, 0)))
            {
                ImGui::OpenPopup("Polyphony Config");
            }
        }

        // Polyphony configuration popup
        if (ImGui::BeginPopup("Polyphony Config"))
        {
            ImGui::SeparatorText("Polyphony Configuration");
            {
                ImGui::Text("Max polyphony:");
                ImGui::Dummy(ImVec2(0, 2));

                if (ImGui::SliderInt("##PolyphonySlider", reinterpret_cast<int*>(&ui->fMaxPolyphony), 1, 16, ui->fMaxPolyphony <= 1 ? "Monopoly" : "%d"))
                {
                    _triggerParamUpdate(pMaxPolyphony, static_cast<float>(ui->fMaxPolyphony));
                }
            }
            ImGui::Dummy(ImVec2(0, 2));
            {
                if (ImGui::Button("OK", ImVec2(70, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine(0, 18);

                if (ImGui::Button("Set to Monopoly", ImVec2(120, 0)))
                {
                    _triggerParamUpdate(pMaxPolyphony, 1);
                }
            }
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::Separator();
            if (ImGui::Checkbox("Arpeggio in polyphony", &ui->fArpPoly))
            {
                _triggerParamUpdate(pArpPoly, ui->fArpPoly ? 1.0f : 0.0f);
            }

            ImGui::EndPopup();
        }
#endif

        ImGui::End();
    }
}

void ImGuiUI::_triggerParamUpdate(uint32_t paramId, float newValue)
{
    ui->setParameterValue(paramId, newValue);   // Tell the DSP to update parameter value
    ui->parameterChanged(paramId, newValue);    // Request UI refresh
}
