#include "CetoneUI.hpp"
#include "Defines.h"
#include "Structures.h"
#include "Images/CetoneArtwork.hpp"
#include "Fonts/CetoneFonts.hpp"

namespace Art = CetoneArtwork;
namespace Fonts = CetoneFonts;

CCetoneUI::CCetoneUI()
    : DISTRHO::UI(Art::guiWidth, Art::guiHeight)
    , fImgBackground(Art::guiData, Art::guiWidth, Art::guiHeight, kImageFormatBGR)
    , fImgKnob(Art::knobData, Art::knobWidth, Art::knobHeight, kImageFormatBGRA)
    , fImgSwitchButton_ON(Art::buttons_onData, Art::buttons_onWidth, Art::buttons_onHeight, kImageFormatBGR)
    , fImgSwitchButton_OFF(Art::buttons_offData, Art::buttons_offWidth, Art::buttons_offHeight, kImageFormatBGR)
{
    /* Initialize NanoVG font and text buffer */
    NanoVG::FontId font = fNanoText.createFontFromMemory("Source Sans Regular", Fonts::SourceSans3_RegularData, Fonts::SourceSans3_RegularDataSize, false);
    fNanoText.fontFaceId(font);
    memset(fLabelBuffer, '\0', sizeof(char) * (32 + 1));

    /* Initialize knobs */
    // NOTICE: Default values comes from CCetoneSynth::InitSynthParameters().
    //         To fit with DSP side on actual run, some values here are different from InitSynthParameters().
    _createKnob(fKnobVolume, pVolume, 260, 40, 0.2f);
    _createKnob(fKnobPanning, pPanning, 260 + 48, 40, 0.5f);
    _createKnob(fKnobOsc1Volume, pOsc1Volume, 260 + 48 * 2, 40, 0.2f);
    _createKnob(fKnobOsc2Volume, pOsc2Volume, 260 + 48 * 3, 40, 0.2f);
    _createKnob(fKnobOsc3Volume, pOsc3Volume, 260 + 48 * 4, 40, 0.2f);

    _createKnob(fKnobFilterType, pFilterType, 512, 40, _pi2f(FTYPE_NONE, FTYPE_MAX));
    _createKnob(fKnobFilterMode, pFilterMode, 512 + 48, 40, _pi2f(FMODE_LOW, FMODE_MAX));
    _createKnob(fKnobCutoff, pCutoff, 512 + 48 * 2, 40, 1.0f);
    _createKnob(fKnobResonance, pResonance, 512 + 48 * 3, 40, 0.0f);
    _createKnob(fKnobFilterParameter, pFilterMod, 512 + 48 * 4, 40, 0.5f);

    _createKnob(fKnobCoarse, pCoarse, 764, 40, 0.5f);
    _createKnob(fKnobFine, pFine, 764 + 48, 40, 0.5f);

    _createKnob(fKnobOsc1Coarse, pOsc1Coarse, 8, 150, 0.5f);    // 0
    _createKnob(fKnobOsc1Fine, pOsc1Fine, 8 + 48, 150, 0.5f);   // 0
    _createKnob(fKnobOsc1Waveform, pOsc1Wave, 8 + 48 * 2, 150, _pi2f(OWAVE_SAW, OWAVE_MAX));
    _createKnob(fKnobOsc1PulseWidth, pOsc1Pw, 8 + 48 * 3, 150, 0.5f);   // 32768

    _createKnob(fKnobOsc2Coarse, pOsc2Coarse, 260, 150, 0.62f);  // +12
    _createKnob(fKnobOsc2Fine, pOsc2Fine, 260 + 48, 150, 0.5f); // 0
    _createKnob(fKnobOsc2Waveform, pOsc2Wave, 260 + 48 * 2, 150, _pi2f(OWAVE_SAW, OWAVE_MAX));
    _createKnob(fKnobOsc2PulseWidth, pOsc2Pw, 260 + 48 * 3, 150, 0.5f); // 32768

    _createKnob(fKnobOsc3Coarse, pOsc3Coarse, 512, 150, 0.38f); // -12
    _createKnob(fKnobOsc3Fine, pOsc3Fine, 512 + 48, 150, 0.5f); // 0
    _createKnob(fKnobOsc3Waveform, pOsc3Wave, 512 + 48 * 2, 150, _pi2f(OWAVE_SAW, OWAVE_MAX));
    _createKnob(fKnobOsc3PulseWidth, pOsc3Pw, 512 + 48 * 3, 150, 0.5f); // 32768

    _createKnob(fKnobGlideSpeed, pPortaSpeed, 764 + 48, 150, 0.1f);

    _createKnob(fAmpAttack, pEnv1A, 8, 260, 0.001f);
    _createKnob(fAmpHold, pEnv1H, 8 + 48, 260, 0.002f);
    _createKnob(fAmpDecay, pEnv1D, 8 + 48 * 2, 260, 0.023f);
    _createKnob(fAmpSustain, pEnv1S, 8 + 48 * 3, 260, 0.75f);
    _createKnob(fAmpRelease, pEnv1R, 8 + 48 * 4, 260, 0.05f);

    _createKnob(fModAttack, pEnv2A, 260, 260, 0.f);
    _createKnob(fModHold, pEnv2H, 260 + 48, 260, 0.f);
    _createKnob(fModDecay, pEnv2D, 260 + 48 * 2, 260, 0.f);
    _createKnob(fModSustain, pEnv2S, 260 + 48 * 3, 260, 0.f);
    _createKnob(fModRelease, pEnv2R, 260 + 48 * 4, 260, 0.f);

    _createKnob(fLfoSpeed, pLfo1Speed, 532, 260, 0.001f);
    _createKnob(fLfoWaveform, pLfo1Wave, 532 + 48, 260, _pi2f(WAVE_SINE, WAVE_MAX));
    _createKnob(fLfoPulseWidth, pLfo1Pw, 532 + 48 * 2, 260, 0.5f);   // 32768

    constexpr auto DEFAULT_ARP_MODE = -1;
    _createKnob(fArpMode, pArpMode, 764, 260, _pi2f(DEFAULT_ARP_MODE + 1, ARP_MAX + 1));    // See CCetoneSynth::getParameter();
    _createKnob(fArpSpeed, pArpSpeed, 764 + 48, 260, 0.96f); // 20

    _createKnob(fMod1Source, pMod1Src, 8, 370, 0.f);
    _createKnob(fMod1Destination, pMod1Dest, 8 + 48, 370, 0.f);
    _createKnob(fMod1Amount, pMod1Amount, 8 + 48 * 2, 370, 0.5f);
    _createKnob(fMod1Multiply, pMod1Mul, 8 + 48 * 3, 370, 0.01f);

    _createKnob(fMod2Source, pMod2Src, 228, 370, 0.f);
    _createKnob(fMod2Destination, pMod2Dest, 228 + 48, 370, 0.f);
    _createKnob(fMod2Amount, pMod2Amount, 228 + 48 * 2, 370, 0.5f);
    _createKnob(fMod2Multiply, pMod2Mul, 228 + 48 * 3, 370, 0.01f);

    _createKnob(fMod3Source, pMod3Src, 448, 370, 0.f);
    _createKnob(fMod3Destination, pMod3Dest, 448 + 48, 370, 0.f);
    _createKnob(fMod3Amount, pMod3Amount, 448 + 48 * 2, 370, 0.5f);
    _createKnob(fMod3Multiply, pMod3Mul, 448 + 48 * 3, 370, 0.01f);

    _createKnob(fMod4Source, pMod4Src, 668, 370, 0.f);
    _createKnob(fMod4Destination, pMod4Dest, 668 + 48, 370, 0.f);
    _createKnob(fMod4Amount, pMod4Amount, 668 + 48 * 2, 370, 0.5f);
    _createKnob(fMod4Multiply, pMod4Mul, 668 + 48 * 3, 370, 0.01f);

    /* Initialize switches */
    _createSwitchButton(fBtnOsc1Ring, pOsc1Ring, 200, 150);
    _createSwitchButton(fBtnOsc2Ring, pOsc2Ring, 260 + 48 * 4, 150);
    _createSwitchButton(fBtnOsc3Ring, pOsc3Ring, 512 + 48 * 4, 150);

    _createSwitchButton(fBtnOsc1Sync, pOsc1Sync, 200, 200);
    _createSwitchButton(fBtnOsc2Sync, pOsc2Sync, 260 + 48 * 4, 200);
    _createSwitchButton(fBtnOsc3Sync, pOsc3Sync, 512 + 48 * 4, 200);

    _createSwitchButton(fBtnGlideState, pPortaMode, 764, 150);

    _createSwitchButton(fBtnLFOTrigger, pLfo1Trig, 677, 260);
}

void CCetoneUI::parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
        // Mixer
        case pVolume:
            fKnobVolume->setValue(value);
            break;
        case pPanning:
            fKnobPanning->setValue(value);
            break;
        case pOsc1Volume:
            fKnobOsc1Volume->setValue(value);
            break;
        case pOsc2Volume:
            fKnobOsc2Volume->setValue(value);
            break;
        case pOsc3Volume:
            fKnobOsc3Volume->setValue(value);
            break;

        // Filter
        case pFilterType:
            fKnobFilterType->setValue(value);
            break;
        case pFilterMode:
            fKnobFilterMode->setValue(value);
            break;
        case pCutoff:
            fKnobCutoff->setValue(value);
            break;
        case pResonance:
            fKnobResonance->setValue(value);
            break;
        case pFilterMod:
            fKnobFilterParameter->setValue(value);
            break;

        // Oscillator 1
        case pOsc1Coarse:
            fKnobOsc1Coarse->setValue(value);
            break;
        case pOsc1Fine:
            fKnobOsc1Fine->setValue(value);
            break;
        case pOsc1Wave:
            fKnobOsc1Waveform->setValue(value);
            break;
        case pOsc1Pw:
            fKnobOsc1PulseWidth->setValue(value);
            break;
        case pOsc1Ring:
            fBtnOsc1Ring->setDown(value == 1.0f ? true : false);
            break;
        case pOsc1Sync:
            fBtnOsc1Sync->setDown(value == 1.0f ? true : false);
            break;

        // Oscillator 2
        case pOsc2Coarse:
            fKnobOsc2Coarse->setValue(value);
            break;
        case pOsc2Fine:
            fKnobOsc2Fine->setValue(value);
            break;
        case pOsc2Wave:
            fKnobOsc2Waveform->setValue(value);
            break;
        case pOsc2Pw:
            fKnobOsc2PulseWidth->setValue(value);
            break;
        case pOsc2Ring:
            fBtnOsc2Ring->setDown(value == 1.0f ? true : false);
            break;
        case pOsc2Sync:
            fBtnOsc2Sync->setDown(value == 1.0f ? true : false);
            break;

        // Oscillator 3
        case pOsc3Coarse:
            fKnobOsc3Coarse->setValue(value);
            break;
        case pOsc3Fine:
            fKnobOsc3Fine->setValue(value);
            break;
        case pOsc3Wave:
            fKnobOsc3Waveform->setValue(value);
            break;
        case pOsc3Pw:
            fKnobOsc3PulseWidth->setValue(value);
            break;
        case pOsc3Ring:
            fBtnOsc3Ring->setDown(value == 1.0f ? true : false);
            break;
        case pOsc3Sync:
            fBtnOsc3Sync->setDown(value == 1.0f ? true : false);
            break;

        // Tuning
        case pCoarse:
            fKnobCoarse->setValue(value);
            break;
        case pFine:
            fKnobFine->setValue(value);
            break;

        // Glide (portamento)
        case pPortaMode:
            fBtnGlideState->setDown(value == 1.0f ? true : false);
            break;
        case pPortaSpeed:
            fKnobGlideSpeed->setValue(value);
            break;

        // Amplifier Envelope
        case pEnv1A:
            fAmpAttack->setValue(value);
            break;
        case pEnv1H:
            fAmpHold->setValue(value);
            break;
        case pEnv1D:
            fAmpDecay->setValue(value);
            break;
        case pEnv1S:
            fAmpSustain->setValue(value);
            break;
        case pEnv1R:
            fAmpRelease->setValue(value);
            break;

        // Modulation Envelope
        case pEnv2A:
            fModAttack->setValue(value);
            break;
        case pEnv2H:
            fModHold->setValue(value);
            break;
        case pEnv2D:
            fModDecay->setValue(value);
            break;
        case pEnv2S:
            fModSustain->setValue(value);
            break;
        case pEnv2R:
            fModRelease->setValue(value);
            break;                                            

        // LFO
        case pLfo1Speed:
            fLfoSpeed->setValue(value);
            break;
        case pLfo1Wave:
            fLfoWaveform->setValue(value);
            break;
        case pLfo1Pw:
            fLfoPulseWidth->setValue(value);
            break;
        case pLfo1Trig:
            fBtnLFOTrigger->setDown(value == 1.0f ? true : false);
            break;

        // Arp
        case pArpMode:
            fArpMode->setValue(value);
            break;
        case pArpSpeed:
            fArpSpeed->setValue(value);
            break;

        // Modulation Slot 1
        case pMod1Src:
            fMod1Source->setValue(value);
            break;
        case pMod1Dest:
            fMod1Destination->setValue(value);
            break;
        case pMod1Amount:
            fMod1Amount->setValue(value);
            break;
        case pMod1Mul:
            fMod1Multiply->setValue(value);
            break;

        // Modulation Slot 2
        case pMod2Src:
            fMod2Source->setValue(value);
            break;
        case pMod2Dest:
            fMod2Destination->setValue(value);
            break;
        case pMod2Amount:
            fMod2Amount->setValue(value);
            break;
        case pMod2Mul:
            fMod2Multiply->setValue(value);
            break;

        // Modulation Slot 3
        case pMod3Src:
            fMod3Source->setValue(value);
            break;
        case pMod3Dest:
            fMod3Destination->setValue(value);
            break;
        case pMod3Amount:
            fMod3Amount->setValue(value);
            break;
        case pMod3Mul:
            fMod3Multiply->setValue(value);
            break;

        // Modulation Slot 4
        case pMod4Src:
            fMod4Source->setValue(value);
            break;
        case pMod4Dest:
            fMod4Destination->setValue(value);
            break;
        case pMod4Amount:
            fMod4Amount->setValue(value);
            break;
        case pMod4Mul:
            fMod4Multiply->setValue(value);
            break;

        default:
            d_stderr2("WARNING: unrecognized parameter %d", index);
            break;
    }

    // Explicitly ask DPF to redraw UI (for updating labels)
    repaint();
}

// -------------------------------------------------------------------
// Widget Callbacks

void CCetoneUI::imageButtonClicked(ImageButton* button, int)
{
#if 0
    switch (button->getId()) {
    case BTN_PANIC: {
        panic();
        break;
    }
    }
#endif
}

void CCetoneUI::imageSwitchClicked(ImageSwitch* button, bool down)
{
    setParameterValue(button->getId(), down);
}

void CCetoneUI::imageKnobDragStarted(ImageKnob* knob)
{
    editParameter(knob->getId(), true);
}

void CCetoneUI::imageKnobDragFinished(ImageKnob* knob)
{
    editParameter(knob->getId(), false);
}

void CCetoneUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    setParameterValue(knob->getId(), value);

    // Explicitly ask DPF to redraw UI (for updating labels)
    repaint();
}

void CCetoneUI::imageSliderDragStarted(ImageSlider* slider)
{
    editParameter(slider->getId(), true);
}

void CCetoneUI::imageSliderDragFinished(ImageSlider* slider)
{
    editParameter(slider->getId(), false);
}

void CCetoneUI::imageSliderValueChanged(ImageSlider* slider, float value)
{
    setParameterValue(slider->getId(), value);
}

void CCetoneUI::onDisplay()
{
    /**
     * Draw background
     */ 
    const GraphicsContext& context(getGraphicsContext());

    fImgBackground.draw(context);

    /**
     * Draw labels
     * NOTICE: Must invoke `repaint()` when tuning widgets, otherwise UI won't update.
     */
    constexpr float r = 255.0f;
    constexpr float g = 255.0f;
    constexpr float b = 255.0f;

    fNanoText.beginFrame(this);
    fNanoText.fontSize(18);
    fNanoText.textAlign(NanoVG::ALIGN_CENTER | NanoVG::ALIGN_MIDDLE);

    fNanoText.fillColor(Color(r, g, b));

    /* Mixer */
    // Volume
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobVolume->getValue() * 100.0f);
    fNanoText.textBox(262, 96, 45.0f, fLabelBuffer);

    // Panning
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobPanning->getValue() * 100.0f - 50.0f);
    fNanoText.textBox(262 + 48, 96, 45.0f, fLabelBuffer);

    // OSC Volumes
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobOsc1Volume->getValue() * 100.0f);
    fNanoText.textBox(262 + 48 * 2, 96, 45.0f, fLabelBuffer);

    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobOsc2Volume->getValue() * 100.0f);
    fNanoText.textBox(262 + 48 * 3, 96, 45.0f, fLabelBuffer);

    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobOsc3Volume->getValue() * 100.0f);
    fNanoText.textBox(262 + 48 * 4, 96, 45.0f, fLabelBuffer);

    /* Filter */
    // Type
    std::snprintf(fLabelBuffer, 32, "%s", _filterType2Str(_pf2i(fKnobFilterType->getValue(), FTYPE_MAX)));
    fNanoText.textBox(514, 96, 45.0f, fLabelBuffer);

    // Mode
    // FIXME: Some filter types does not support setting mode.
    const auto _filterMode = _pf2i(fKnobFilterMode->getValue(), FMODE_MAX);
    std::snprintf(fLabelBuffer, 32, "%s", _filterMode2str(_filterMode));
    fNanoText.textBox(514 + 48, 96, 45.0f, fLabelBuffer);

    // Cutoff
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobCutoff->getValue() * 100.0f);
    fNanoText.textBox(514 + 48 * 2, 96, 45.0f, fLabelBuffer);

    // Resonance
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobResonance->getValue() * 100.0f);
    fNanoText.textBox(514 + 48 * 3, 96, 45.0f, fLabelBuffer);

    // Filter Parameter (Filter Mod)
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobFilterParameter->getValue() * 100.0f);
    fNanoText.textBox(514 + 48 * 4, 96, 45.0f, fLabelBuffer);

    /* Tuning */
    // Coarse
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2coarse(fKnobCoarse->getValue()));
    fNanoText.textBox(766, 96, 45.0f, fLabelBuffer);

    // Fine
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2fine(fKnobFine->getValue()));
    fNanoText.textBox(766 + 48, 96, 45.0f, fLabelBuffer); 

    /* Oscillator 1 */
    // Osc1 Coarse
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2coarse(fKnobOsc1Coarse->getValue()));
    fNanoText.textBox(10, 206, 45.0f, fLabelBuffer);

    // Osc1 Fine
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2fine(fKnobOsc1Fine->getValue()));
    fNanoText.textBox(10 + 48, 206, 45.0f, fLabelBuffer); 

    // Osc1 Waveform
    std::snprintf(fLabelBuffer, 32, "%s", _OscWave2Str(_pf2i(fKnobOsc1Waveform->getValue(), OWAVE_MAX)));
    fNanoText.textBox(10 + 48 * 2, 206, 45.0f, fLabelBuffer);

    // Osc1 Pulse Width
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2pw(fKnobOsc1PulseWidth->getValue()));
    fNanoText.textBox(10 + 48 * 3, 206, 45.0f, fLabelBuffer);

    /* Oscillator 2 */
    // Osc2 Coarse
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2coarse(fKnobOsc2Coarse->getValue()));
    fNanoText.textBox(262, 206, 45.0f, fLabelBuffer);

    // Osc2 Fine
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2fine(fKnobOsc2Fine->getValue()));
    fNanoText.textBox(262 + 48, 206, 45.0f, fLabelBuffer); 

    // Osc2 Waveform
    std::snprintf(fLabelBuffer, 32, "%s", _OscWave2Str(_pf2i(fKnobOsc2Waveform->getValue(), OWAVE_MAX)));
    fNanoText.textBox(262 + 48 * 2, 206, 45.0f, fLabelBuffer);

    // Osc2 Pulse Width
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2pw(fKnobOsc2PulseWidth->getValue()));
    fNanoText.textBox(262 + 48 * 3, 206, 45.0f, fLabelBuffer);

    /* Oscillator 3 */
    // Osc3 Coarse
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2coarse(fKnobOsc3Coarse->getValue()));
    fNanoText.textBox(514, 206, 45.0f, fLabelBuffer);

    // Osc3 Fine
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2fine(fKnobOsc3Fine->getValue()));
    fNanoText.textBox(514 + 48, 206, 45.0f, fLabelBuffer); 

    // Osc3 Waveform
    std::snprintf(fLabelBuffer, 32, "%s", _OscWave2Str(_pf2i(fKnobOsc3Waveform->getValue(), OWAVE_MAX)));
    fNanoText.textBox(514 + 48 * 2, 206, 45.0f, fLabelBuffer);

    // Osc3 Pulse Width
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2pw(fKnobOsc3PulseWidth->getValue()));
    fNanoText.textBox(514 + 48 * 3, 206, 45.0f, fLabelBuffer);

    /* Glide (portamento) */
    // Speed
    std::snprintf(fLabelBuffer, 32, "%.1f", fKnobGlideSpeed->getValue() * 100.0f);
    fNanoText.textBox(766 + 48, 206, 45.0f, fLabelBuffer); 

    /* Amplifier Envelope */
    // Attack
    std::snprintf(fLabelBuffer, 32, "%.2f", fAmpAttack->getValue() * 100.0f);
    fNanoText.textBox(10, 316, 45.0f, fLabelBuffer);

    // Hold
    std::snprintf(fLabelBuffer, 32, "%.2f", fAmpHold->getValue() * 100.0f);
    fNanoText.textBox(10 + 48, 316, 45.0f, fLabelBuffer);

    // Decay
    std::snprintf(fLabelBuffer, 32, "%.2f", fAmpDecay->getValue() * 100.0f);
    fNanoText.textBox(10 + 48 * 2, 316, 45.0f, fLabelBuffer);

    // Sustain
    std::snprintf(fLabelBuffer, 32, "%.2f", fAmpSustain->getValue() * 100.0f);
    fNanoText.textBox(10 + 48 * 3, 316, 45.0f, fLabelBuffer);

    // Release
    std::snprintf(fLabelBuffer, 32, "%.2f", fAmpRelease->getValue() * 100.0f);
    fNanoText.textBox(10 + 48 * 4, 316, 45.0f, fLabelBuffer);

    /* Modulation Envelope */
    // Attack
    std::snprintf(fLabelBuffer, 32, "%.2f", fModAttack->getValue() * 100.0f);
    fNanoText.textBox(262, 316, 45.0f, fLabelBuffer);

    // Hold
    std::snprintf(fLabelBuffer, 32, "%.2f", fModHold->getValue() * 100.0f);
    fNanoText.textBox(262 + 48, 316, 45.0f, fLabelBuffer);

    // Decay
    std::snprintf(fLabelBuffer, 32, "%.2f", fModDecay->getValue() * 100.0f);
    fNanoText.textBox(262 + 48 * 2, 316, 45.0f, fLabelBuffer);

    // Sustain
    std::snprintf(fLabelBuffer, 32, "%.2f", fModSustain->getValue() * 100.0f);
    fNanoText.textBox(262 + 48 * 3, 316, 45.0f, fLabelBuffer);

    // Release
    std::snprintf(fLabelBuffer, 32, "%.2f", fModRelease->getValue() * 100.0f);
    fNanoText.textBox(262 + 48 * 4, 316, 45.0f, fLabelBuffer);

    /* LFO */
    // Speed
    std::snprintf(fLabelBuffer, 32, "%.2f", fLfoSpeed->getValue() * 100.0f);
    fNanoText.textBox(534, 316, 45.0f, fLabelBuffer);

    // Waveform
    std::snprintf(fLabelBuffer, 32, "%s", _wave2Str(_pf2i(fLfoWaveform->getValue(), WAVE_MAX)));
    fNanoText.textBox(534 + 48, 316, 45.0f, fLabelBuffer);

    // Pulse Width
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2pw(fLfoPulseWidth->getValue()));
    fNanoText.textBox(534 + 48 * 2, 316, 45.0f, fLabelBuffer);

    /* Arp */
    // Mode
    std::snprintf(fLabelBuffer, 32, "%s", _arpMode2str(_pf2i(fArpMode->getValue(), ARP_MAX + 1) - 1));
    fNanoText.textBox(767, 316, 45.0f, fLabelBuffer);

    // Speed
    std::snprintf(fLabelBuffer, 32, "%d", (int)((1.f - fArpSpeed->getValue()) * 500.f + 0.5f));
    fNanoText.textBox(767 + 48, 316, 45.0f, fLabelBuffer);

    /* Modulation Slot 1 */
    // Source
    std::snprintf(fLabelBuffer, 32, "%s", _modSrc2str(_pf2i(fMod1Source->getValue(), MOD_SRC_MAX)));
    fNanoText.textBox(10, 426, 45.0f, fLabelBuffer);

    // Destination
    std::snprintf(fLabelBuffer, 32, "%s", _modDest2str(_pf2i(fMod1Destination->getValue(), MOD_DEST_MAX)));
    fNanoText.textBox(10 + 48, 426, 48.0f, fLabelBuffer);    

    // Amount
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modAmount(fMod1Amount->getValue()));
    fNanoText.textBox(10 + 48 * 2, 426, 45.0f, fLabelBuffer);

    // Multiplicator
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modMul(fMod1Multiply->getValue()));
    fNanoText.textBox(10 + 48 * 3, 426, 45.0f, fLabelBuffer);    

    /* Modulation Slot 2 */
    // Source
    std::snprintf(fLabelBuffer, 32, "%s", _modSrc2str(_pf2i(fMod2Source->getValue(), MOD_SRC_MAX)));
    fNanoText.textBox(230, 426, 45.0f, fLabelBuffer);

    // Destination
    std::snprintf(fLabelBuffer, 32, "%s", _modDest2str(_pf2i(fMod2Destination->getValue(), MOD_DEST_MAX)));
    fNanoText.textBox(230 + 48, 426, 48.0f, fLabelBuffer);    

    // Amount
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modAmount(fMod2Amount->getValue()));
    fNanoText.textBox(230 + 48 * 2, 426, 45.0f, fLabelBuffer);

    // Multiplicator
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modMul(fMod2Multiply->getValue()));
    fNanoText.textBox(230 + 48 * 3, 426, 45.0f, fLabelBuffer);    

    /* Modulation Slot 3 */
    // Source
    std::snprintf(fLabelBuffer, 32, "%s", _modSrc2str(_pf2i(fMod3Source->getValue(), MOD_SRC_MAX)));
    fNanoText.textBox(450, 426, 45.0f, fLabelBuffer);

    // Destination
    std::snprintf(fLabelBuffer, 32, "%s", _modDest2str(_pf2i(fMod3Destination->getValue(), MOD_DEST_MAX)));
    fNanoText.textBox(450 + 48, 426, 48.0f, fLabelBuffer);    

    // Amount
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modAmount(fMod3Amount->getValue()));
    fNanoText.textBox(450 + 48 * 2, 426, 45.0f, fLabelBuffer);

    // Multiplicator
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modMul(fMod3Multiply->getValue()));
    fNanoText.textBox(450 + 48 * 3, 426, 45.0f, fLabelBuffer);   

    /* Modulation Slot 4 */
    // Source
    std::snprintf(fLabelBuffer, 32, "%s", _modSrc2str(_pf2i(fMod4Source->getValue(), MOD_SRC_MAX)));
    fNanoText.textBox(670, 426, 45.0f, fLabelBuffer);

    // Destination
    std::snprintf(fLabelBuffer, 32, "%s", _modDest2str(_pf2i(fMod4Destination->getValue(), MOD_DEST_MAX)));
    fNanoText.textBox(670 + 48, 426, 48.0f, fLabelBuffer);    

    // Amount
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modAmount(fMod4Amount->getValue()));
    fNanoText.textBox(670 + 48 * 2, 426, 45.0f, fLabelBuffer);

    // Multiplicator
    std::snprintf(fLabelBuffer, 32, "%d", _c_val2modMul(fMod4Multiply->getValue()));
    fNanoText.textBox(670 + 48 * 3, 426, 45.0f, fLabelBuffer);   

    fNanoText.endFrame();
}

void CCetoneUI::idleCallback() { }

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new CCetoneUI();
}

END_NAMESPACE_DISTRHO
