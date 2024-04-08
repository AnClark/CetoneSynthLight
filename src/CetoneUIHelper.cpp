#include "CetoneUI.hpp"
#include "Defines.h"

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
