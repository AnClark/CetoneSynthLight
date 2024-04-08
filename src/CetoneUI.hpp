#pragma once

#include "DistrhoUI.hpp"
#include "ImageWidgets.hpp"
#include "NanoVG.hpp"

using DGL_NAMESPACE::ImageAboutWindow;
using DGL_NAMESPACE::ImageButton;
using DGL_NAMESPACE::ImageKnob;
using DGL_NAMESPACE::ImageSlider;
using DGL_NAMESPACE::ImageSwitch;

class MinatonPresetManager;

// -----------------------------------------------------------------------

class CCetoneUI : public DISTRHO::UI,
                  public ImageButton::Callback,
                  public ImageKnob::Callback,
                  public ImageSlider::Callback,
                  public ImageSwitch::Callback,
                  public IdleCallback {
public:
    CCetoneUI();

protected:
    // -------------------------------------------------------------------
    // DSP Callbacks

    void parameterChanged(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int) override;
    void imageSwitchClicked(ImageSwitch* button, bool) override;
    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;
    void imageSliderDragStarted(ImageSlider* slider) override;
    void imageSliderDragFinished(ImageSlider* slider) override;
    void imageSliderValueChanged(ImageSlider* slider, float value) override;

    void onDisplay() override;

    // -------------------------------------------------------------------
    // Other Callbacks

    void idleCallback() override;

private:
    // -------------------------------------------------------------------
    // Label renderer

    NanoVG fNanoText;
    char fLabelBuffer[32 + 1];

    // -------------------------------------------------------------------
    // Image resources

    Image fImgBackground;
    Image fImgKnob;
    Image fImgSwitchButton_ON, fImgSwitchButton_OFF;

    // -------------------------------------------------------------------
    // Widgets

    ScopedPointer<ImageKnob> fKnobVolume, fKnobPanning, fKnobOsc1Volume, fKnobOsc2Volume, fKnobOsc3Volume;
    ScopedPointer<ImageKnob> fKnobFilterType, fKnobFilterMode, fKnobCutoff, fKnobResonance, fKnobFilterParameter;
    ScopedPointer<ImageKnob> fKnobCoarse, fKnobFine;

    ScopedPointer<ImageKnob> fKnobOsc1Coarse, fKnobOsc1Fine, fKnobOsc1Waveform, fKnobOsc1PulseWidth;
    ScopedPointer<ImageKnob> fKnobOsc2Coarse, fKnobOsc2Fine, fKnobOsc2Waveform, fKnobOsc2PulseWidth;
    ScopedPointer<ImageKnob> fKnobOsc3Coarse, fKnobOsc3Fine, fKnobOsc3Waveform, fKnobOsc3PulseWidth;

    ScopedPointer<ImageKnob> fKnobGlideSpeed;

    ScopedPointer<ImageKnob> fAmpAttack, fAmpHold, fAmpDecay, fAmpSustain, fAmpRelease;
    ScopedPointer<ImageKnob> fModAttack, fModHold, fModDecay, fModSustain, fModRelease;

    ScopedPointer<ImageKnob> fLfoSpeed, fLfoWaveform, fLfoPulseWidth;

    ScopedPointer<ImageKnob> fArpMode, fArpSpeed;

    ScopedPointer<ImageKnob> fMod1Source, fMod1Destination, fMod1Amount, fMod1Multiply;
    ScopedPointer<ImageKnob> fMod2Source, fMod2Destination, fMod2Amount, fMod2Multiply;
    ScopedPointer<ImageKnob> fMod3Source, fMod3Destination, fMod3Amount, fMod3Multiply;
    ScopedPointer<ImageKnob> fMod4Source, fMod4Destination, fMod4Amount, fMod4Multiply;

    ScopedPointer<ImageSwitch> fBtnOsc1Ring, fBtnOsc2Ring, fBtnOsc3Ring;
    ScopedPointer<ImageSwitch> fBtnOsc1Sync, fBtnOsc2Sync, fBtnOsc3Sync;
    ScopedPointer<ImageSwitch> fBtnGlideState;
    ScopedPointer<ImageSwitch> fBtnLFOTrigger;

    // -------------------------------------------------------------------
    // Helpers

    void _createKnob(ScopedPointer<ImageKnob>& knob, uint32_t paramId, uint absolutePosX, uint absolutePosY, float defaultValue, uint rotationAngle = 275);
    void _createSlider(ScopedPointer<ImageSlider>& slider, uint32_t paramId, uint startPosX, uint startPosY, uint endPosX, uint endPosY, float step, bool inverted = false);
    void _createSwitchButton(ScopedPointer<ImageSwitch>& switchButton, uint32_t paramId, uint absolutePosX, uint absolutePosY);
    void _createButton(ScopedPointer<ImageButton>& button, uint id, Image& imageNormal, Image& imagePressed, uint absolutePosX, uint absolutePosY);

    const char* _wave2Str(int wave);
    const char* _OscWave2Str(int wave);
    const char* _filterType2Str(int type);
    const char* _filterMode2str(int mode);
    const char* _modSrc2str(int val);
    const char* _modDest2str(int val);
    const char* _arpMode2str(int val);

    int _pf2i(float val, int max);
    float _pi2f(int val, int max);
    int _c_val2coarse(float value);
    int _c_val2fine(float value);
    int _c_val2pw(float value);
    int _c_val2modAmount(float value);
    int _c_val2modMul(float value);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CCetoneUI)
};

// -----------------------------------------------------------------------

// --------------------------------
// Button IDs

constexpr uint BTN_PANIC = d_cconst('p', 'n', 'i', 'c');

// -----------------------------------------------------------------------
