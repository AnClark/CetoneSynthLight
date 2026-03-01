#include "CetoneUI.hpp"
#include "Defines.h"
#include "Structures.h"

#include <cstdarg>
#include <cstdio>
#include <map>
#include <vector>

constexpr float PARAM_MIN_VALUE = 0.0f;
constexpr float PARAM_MAX_VALUE = 1.0f;
constexpr float PARAM_DEFAULT_VALUE = 0.5f;

void CCetoneUI::_createKnob(ScopedPointer<ImageKnob>& knob, uint32_t paramId, uint absolutePosX, uint absolutePosY, float defaultValue, uint rotationAngle)
{
    Image& knob_image = fImgKnob;

    knob = new ImageKnob(this, knob_image, ImageKnob::Vertical);
    knob->setId(paramId);
    knob->setAbsolutePos(absolutePosX, absolutePosY);
    knob->setRange(PARAM_MIN_VALUE, PARAM_MAX_VALUE);
    knob->setDefault(defaultValue);
    knob->setValue(defaultValue);
    knob->setRotationAngle(rotationAngle);
    knob->setCallback(this);
}

void CCetoneUI::_createSlider(ScopedPointer<ImageSlider>& slider, uint32_t paramId, uint startPosX, uint startPosY, uint endPosX, uint endPosY, float step, bool inverted)
{
#if 0
    slider = new ImageSlider(this, fSliderImage);
    slider->setId(paramId);
    slider->setStartPos(startPosX, startPosY);
    slider->setEndPos(endPosX, endPosY);
    slider->setRange(MinatonParams::paramMinValue(paramId), MinatonParams::paramMaxValue(paramId));
    slider->setStep(step);
    slider->setValue(MinatonParams::paramDefaultValue(paramId));
    slider->setInverted(inverted);
    slider->setCallback(this);
#endif
}

void CCetoneUI::_createSwitchButton(ScopedPointer<ImageSwitch>& switchButton, uint32_t paramId, uint absolutePosX, uint absolutePosY)
{
    switchButton = new ImageSwitch(this, fImgSwitchButton_OFF, fImgSwitchButton_ON);
    switchButton->setId(paramId);
    switchButton->setAbsolutePos(absolutePosX, absolutePosY);
    switchButton->setCallback(this);
}

void CCetoneUI::_createButton(ScopedPointer<ImageButton>& button, uint id, Image& imageNormal, Image& imagePressed, uint absolutePosX, uint absolutePosY)
{
    button = new ImageButton(this, imageNormal, imagePressed);
    button->setId(id);
    button->setAbsolutePos(absolutePosX, absolutePosY);
    button->setCallback(this);
}

void CCetoneUI::_createHiddenButton(ScopedPointer<ImageButton>& button, uint id, Size<uint> size, Point<int> absolutePos)
{
    button = new ImageButton(this, fImgTransparent, fImgTransparent);
    button->setId(id);
    button->setAbsolutePos(absolutePos);
    button->setSize(size);
    button->setCallback(this);
}

const char* CCetoneUI::_wave2Str(int wave)
{
    switch (wave) {
    case WAVE_SINE:
        return "Sine";
        break;
    case WAVE_TRI:
        return "Tri";
        break;
    case WAVE_SAW:
        return "Saw";
        break;
    case WAVE_PULSE:
        return "Pulse";
        break;
    case WAVE_C64NOISE:
        return "Noise";
        break;
    default:
        return "Unknown";
        break;
    }
}

const char* CCetoneUI::_OscWave2Str(int wave)
{
    switch (wave) {
    case OWAVE_SINE:
        return "Sine";
        break;
    case OWAVE_TRI:
        return "Tri";
        break;
    case OWAVE_SAW:
        return "Saw";
        break;
    case OWAVE_PULSE:
        return "Pulse";
        break;
    case OWAVE_C64NOISE:
        return "Noise";
        break;
    default:
        return "Unknown";
        break;
    }
}

const char* CCetoneUI::_filterType2Str(int type)
{
    switch (type) {
    case FTYPE_NONE:
        return "None";
        break;
    case FTYPE_DIRTY:
        return "Dirty";
        break;
    case FTYPE_MOOG:
        return "Moog";
        break;
    case FTYPE_MOOG2:
        return "Moog2";
        break;
    case FTYPE_CH12DB:
        return "Ch12db";
        break;
    case FTYPE_303:
        return "x0x";
        break;
    case FTYPE_8580:
        return "8580";
        break;
    case FTYPE_BUDDA:
        return "Bi12db";
        break;
    default:
        return "\0";
        break;
    }
}

const char* CCetoneUI::_filterMode2str(int mode)
{
    switch (mode) {
    case FMODE_LOW:
        return "Low";
        break;
    case FMODE_HIGH:
        return "High";
        break;
    case FMODE_BAND:
        return "Band";
        break;
    case FMODE_NOTCH:
        return "Notch";
        break;
    default:
        return "N/A";
        break;
    }
}

const char* CCetoneUI::_modSrc2str(int val)
{
    switch (val) {
    case MOD_SRC_NONE:
        return "None";
        break;
    case MOD_SRC_VEL:
        return "Vel.";
        break;
    case MOD_SRC_CTRL1:
        return "Ctrl 1";
        break;
    case MOD_SRC_MENV1:
        return "MEnv";
        break;
    case MOD_SRC_LFO1:
        return "LFO";
        break;
    case MOD_SRC_MENV1xLFO1:
        return "ME1xL1";
        break;
    default:
        return "Unknown";
        break;
    }
}

const char* CCetoneUI::_modDest2str(int val)
{
    switch (val) {
    case MOD_DEST_MAINVOL:
        return "Volume";
        break;
    case MOD_DEST_CUTOFF:
        return "Cutoff";
        break;
    case MOD_DEST_RESONANCE:
        return "Q";
        break;
    case MOD_DEST_PANNING:
        return "Pan.";
        break;
    case MOD_DEST_MAINPITCH:
        return "Pitch";
        break;
    case MOD_DEST_OSC1VOL:
        return "Vol 1";
        break;
    case MOD_DEST_OSC2VOL:
        return "Vol 2";
        break;
    case MOD_DEST_OSC3VOL:
        return "Vol 3";
        break;
    case MOD_DEST_OSC1PITCH:
        return "Pitch1";
        break;
    case MOD_DEST_OSC2PITCH:
        return "Pitch2";
        break;
    case MOD_DEST_OSC3PITCH:
        return "Pitch3";
        break;
    case MOD_DEST_OSC1PW:
        return "PW 1";
        break;
    case MOD_DEST_OSC2PW:
        return "PW 2";
        break;
    case MOD_DEST_OSC3PW:
        return "PW 3";
        break;
    case MOD_DEST_LFO1SPEED:
        return "L1Spd.";
        break;
    case MOD_DEST_ENVMOD:
        return "F.Param.";
        break;
    default:
        return "Unknown";
        break;
    }
}

const char* CCetoneUI::_arpMode2str(int val)
{
    switch (val) {
    case -1:
        return "Off";
        break;
    case 0:
        return "Minor";
        break;
    case 1:
        return "Major";
        break;
    case 2:
        return "MinOct";
        break;
    case 3:
        return "MajOct";
        break;
    case 4:
        return "Octave";
        break;
    case 5:
        return "Oct2";
        break;
    case 6:
        return "Quint";
        break;
    case 7:
        return "Quint2";
        break;
    default:
        return "Unknown";
        break;
    }
}

int CCetoneUI::_pf2i(float val, int max)
{
    int tmp = (int)floor(((float)(max + 1) * (float)val) + 0.5f);

    if (tmp < 0)
        tmp = 0;
    else if (tmp > max)
        tmp = max;

    return tmp;
}

float CCetoneUI::_pi2f(int val, int max)
{
    return (float)val / (float)(max + 1);
}

int CCetoneUI::_c_val2coarse(float value)
{
    return (int)(value * 100.f + 0.5f) - 50;
}

int CCetoneUI::_c_val2fine(float value)
{
    return (int)(value * 200.f + 0.5f) - 100;
}

int CCetoneUI::_c_val2pw(float value)
{
    return (int)(value * 65536.f + 0.5f);
}

int CCetoneUI::_c_val2modAmount(float value)
{
    return floorf(value * 200.f + 0.5f) - 100.f;
}

int CCetoneUI::_c_val2modMul(float value)
{
    return floorf(value * 100.f + 0.5f);
}

void CCetoneUI::_requestMessageBox(std::string message)
{
    DISTRHO_SAFE_ASSERT_RETURN(fImGuiInstance.get(), )

    fImGuiInstance->messageBoxQueue.push(std::string(message));
}

void CCetoneUI::logAndShowMessage(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    constexpr uint16_t MAX_MESSAGE_LENGTH = 512;
    char buffer[MAX_MESSAGE_LENGTH] = {'\0'};
    vsnprintf(buffer, MAX_MESSAGE_LENGTH, fmt, args);

    va_end(args);

    // Print log to console
    d_stderr("%s", buffer);

    // Show message box on UI side
    _requestMessageBox(std::string(buffer));
}

void CCetoneUI::_updateState(const char* newPresetName, const char* newBankName, bool isModified)
{
    // Update local storage
    this->fCurrentPresetName = newPresetName;
    this->fCurrentPresetBank = newBankName;
    this->fPresetIsModified = isModified;

    // Send state to DSP side
    this->setState(STATE_PRESET_NAME, newPresetName);
    this->setState(STATE_PRESET_BANK, newBankName);
    this->setState(STATE_PRESET_MODIFIED, isModified ? "true" : "false");

    // Request host to mark project as dirty and enable undo
    _triggerDummyParameterChange();
}

void CCetoneUI::_updateState(bool isModified)
{
    this->fPresetIsModified = isModified;
    this->setState(STATE_PRESET_MODIFIED, isModified ? "true" : "false");

    // NOTE: This function overload is only invoked in widget callbacks when parameters
    //       are changed by user interaction, so we don't need to call
    //       _triggerDummyParameterChange() here.
}

void CCetoneUI::_triggerDummyParameterChange()
{
    // Notify host that parameters have been changed by current preset,
    // so that host can mark project as dirty and enable undo.
    editParameter(0, true);
    editParameter(0, false);
}

bool CCetoneUI::_validatePresetAndBankState(const String& presetName, const String& bankName)
{
    // Single imported preset: lives in memory only (not backed by any bank file).
    // We cannot verify it on disk, so trust whatever the host says.
    if (bankName == BANK_NAME_FOR_SINGLE_IMPORTED_PRESET) {
        return true;
    }

    // Factory bank: only "Init Patch" is valid
    if (bankName == FACTORY_BANK_NAME) {
        return (presetName == DEFAULT_PRESET_NAME);
    }

    std::vector<String> defaultBankPresets;
    std::vector<String> importedBanks;
    std::map<std::string, std::vector<String>> importedBankPresets;

    // Fetch the newest list of presets (default bank)
    for (size_t i = 0; i < fPresetManager->getDefaultBankPresetCount(); i++)
        defaultBankPresets.push_back(fPresetManager->getDefaultBankPresetName(i));

    // Fetch the newest list of banks and presets (imported banks)
    importedBanks = fPresetManager->getImportedBankNames();
    importedBankPresets.clear();
    for (const auto& bank : importedBanks)
        importedBankPresets[bank.buffer()] = fPresetManager->getPresetsInBank(bank.buffer());

    if (bankName == DEFAULT_USER_BANK_NAME) {
        // Check if the specific preset exists in default bank
        for (const auto& preset : defaultBankPresets) {
            if (presetName == preset) {
                return true;
            }
        }
        return false;
    } else {
        // Imported bank preset: check if bank and preset still exist
        auto it = importedBankPresets.find(bankName.buffer());
        if (it != importedBankPresets.end()) {
            const std::vector<String>& presets = it->second;
            for (const auto& preset : presets) {
                if (presetName == preset) {
                    return true;
                }
            }
        }
        return false;
    }
}

void CCetoneUI::_fallbackToDefaultStateOfPreset()
{
    // Correct UI metadata to default state when the host reverts to a snapshot
    // that references a bank or preset that no longer exists on disk.
    fCurrentPresetName = DEFAULT_PRESET_NAME;
    fPresetIsModified  = true; // Parameters no longer match any saved preset
    setState(STATE_PRESET_NAME,     DEFAULT_PRESET_NAME);
    setState(STATE_PRESET_MODIFIED, "true");

    _triggerDummyParameterChange();
}

void CCetoneUI::_fallbackToDefaultStateOfBank()
{
    // Correct UI metadata to default (factory) bank state.
    fCurrentPresetBank = FACTORY_BANK_NAME;
    fPresetIsModified  = true;
    setState(STATE_PRESET_BANK,     FACTORY_BANK_NAME);
    setState(STATE_PRESET_MODIFIED, "true");

    _triggerDummyParameterChange();
}
