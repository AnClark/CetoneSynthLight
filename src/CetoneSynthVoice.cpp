#include "CetoneSynthVoice.h"
#include "SynthOscillator.h"
#include "SynthEnvelope.h"
#include "SynthLfo.h"
#include "FilterDirty.h"
#include "FilterMoog.h"
#include "FilterMoog2.h"
#include "FilterCh12db.h"
#include "Filter303.h"
#include "Filter8580.h"
#include "FilterBiquad.h"
#include "Defines.h"
#include "GlobalFunctions.h"
#include <cmath>

CetoneSynthVoice::CetoneSynthVoice()
{
	for (int i = 0; i < 3; i++)
		this->Oscs[i] = new CSynthOscillator();

	this->Oscs[1]->SetSyncDest(this->Oscs[0]);
	this->Oscs[2]->SetSyncDest(this->Oscs[1]);
	this->Oscs[0]->SetSyncDest(this->Oscs[2]);

	for (int i = 0; i < 2; i++)
		this->Envs[i] = new CSynthEnvelope();

	this->Envs[0]->SetPreAttack(0.02f);
	this->Envs[1]->SetPreAttack(0.002f);

	this->Lfo = new CSynthLfo();
	
	// Create per-voice filters
	this->FilterDirty = new CFilterDirty();
	this->FilterCh12db = new CFilterCh12db();
	this->FilterMoog = new CFilterMoog();
	this->FilterMoog2 = new CFilterMoog2();
	this->Filter303 = new CFilter303();
	this->Filter8580 = new CFilter8580();
	this->FilterBiquad = new CFilterBiquad();
	
	this->filterType = FTYPE_NONE;
	this->filterMode = 0;

	Reset();
}

CetoneSynthVoice::~CetoneSynthVoice()
{
	for (int i = 0; i < 3; i++)
		delete this->Oscs[i];

	for (int i = 0; i < 2; i++)
		delete this->Envs[i];

	delete this->Lfo;
	
	// Delete per-voice filters
	delete this->FilterDirty;
	delete this->FilterCh12db;
	delete this->FilterMoog;
	delete this->FilterMoog2;
	delete this->Filter303;
	delete this->Filter8580;
	delete this->FilterBiquad;
}

void CetoneSynthVoice::Reset()
{
	isActive = false;
	isReleasing = false;
	noteNumber = -1;
	velocity = 0;
	voiceAge = 0;
	currentPitch = 0;
	portaPitch = 0;
	portaFrac = 0;
	portaStep = 0;
	doPorta = false;

	for (int i = 0; i < 3; i++)
		voicePulsewidth[i] = 32768;

	velocityMod = 0.0f;
	velocityModStep = 0.0f;
	velocityModEnd = 0.0f;

	modEnvValue = 0.0f;

	// Reset arpeggiator state
	arpPos = 0;
	arpCounter = 0;
	arpDelay = 0;

	for (int i = 0; i < 3; i++)
		this->Oscs[i]->Reset();

	for (int i = 0; i < 2; i++)
		this->Envs[i]->Reset();

	this->Lfo->Reset();
	
	// Reset per-voice filters
	this->FilterDirty->Reset();
	this->FilterCh12db->Reset();
	this->FilterMoog->Reset();
	this->FilterMoog2->Reset();
	this->Filter303->Reset();
	this->Filter8580->Reset();
	this->FilterBiquad->Reset();
}

float CetoneSynthVoice::GetEnvelopeLevel() const
{
	// Return the amplitude envelope level (used for voice stealing)
	if (!isActive)
		return 0.0f;

	// We can't access the internal envelope level easily,
	// so we use a simple heuristic: releasing voices have lower priority
	return isReleasing ? 0.1f : 1.0f;
}

void CetoneSynthVoice::NoteOn(int note, int vel, bool portamento, int fromPitch, int portaSamples)
{
	noteNumber = note;
	velocity = vel;
	isActive = true;
	isReleasing = false;
	voiceAge = 0;

	velocityModEnd = (float)vel / 127.0f;
	velocityMod = velocityModEnd; // Instant velocity response for now
	velocityModStep = 0.0f;

	int targetPitch = (note + NOTE_OFFSET) * 100;

	if (portamento && fromPitch != 0)
	{
		// Calculate portamento step ONCE in NoteOn (like original monophonic version)
		doPorta = true;
		currentPitch = fromPitch;
		portaPitch = targetPitch;
		portaFrac = currentPitch << 14;
		// Calculate fixed portaStep based on distance and time
		if (portaSamples > 0)
		{
			portaStep = (int)(((targetPitch - fromPitch) / (float)portaSamples) * 16384.0f + 0.5f);
		}
		else
		{
			portaStep = 0;
		}
	}
	else
	{
		doPorta = false;
		currentPitch = targetPitch;
		portaPitch = targetPitch;
		portaStep = 0;
	}

	// Trigger envelopes
	this->Envs[0]->Gate(true);
	this->Envs[1]->Gate(true);
}

void CetoneSynthVoice::NoteOff()
{
	if (!isActive)
		return;

	isReleasing = true;

	// Release envelopes
	this->Envs[0]->Gate(false);
	this->Envs[1]->Gate(false);
}

void CetoneSynthVoice::InitArpeggiator(int delay)
{
	arpPos = 0;
	arpCounter = delay;
	arpDelay = delay;
}

int CetoneSynthVoice::GetArpOffset(int arpMode, const int arpTable[8][16])
{
	if (arpMode == -1 || !isActive)
		return 0;

	// Check if arpeggiator position needs to wrap
	if (arpPos >= arpTable[arpMode][15])
		arpPos = 0;

	// Get current arpeggio offset in semitones
	int offset = arpTable[arpMode][arpPos];

	// Update arpeggiator counter
	arpCounter--;
	if (arpCounter <= 0)
	{
		arpCounter = arpDelay;
		arpPos++;
	}

	return offset;
}

void CetoneSynthVoice::UpdateEnvelopes(float attack0, float hold0, float decay0, float sustain0, float release0,
									   float attack1, float hold1, float decay1, float sustain1, float release1)
{
	this->Envs[0]->Set(attack0, hold0, decay0, sustain0, release0);
	this->Envs[1]->Set(attack1, hold1, decay1, sustain1, release1);
}

void CetoneSynthVoice::SetLfoParams(float speed, int pw, int wave, bool trigger)
{
	this->Lfo->Set(speed, pw, wave, trigger);
}

void CetoneSynthVoice::TriggerLfo()
{
	this->Lfo->Trigger();
}

float CetoneSynthVoice::Render(const SynthVoice voice[3], bool doPortamento, float portaSpeed, int portaSamples, 
								   const VoiceModulation* voiceMod, int arpOffset)
{
	if (!isActive)
		return 0.0f;

	// Handle portamento (using fixed portaStep calculated in NoteOn)
	if (doPorta && doPortamento)
	{
		portaFrac += portaStep;
		int tmp = portaFrac >> 14;

		if (portaStep < 0)
		{
			if (tmp <= portaPitch)
			{
				tmp = portaPitch;
				doPorta = false;
			}
		}
		else
		{
			if (tmp >= portaPitch)
			{
				tmp = portaPitch;
				doPorta = false;
			}
		}

		currentPitch = tmp;
	}

	// Calculate oscillator pitches with voice tuning
	int tune[3];
	tune[0] = voice[0].Coarse * 100 + voice[0].Fine;
	tune[1] = voice[1].Coarse * 100 + voice[1].Fine;
	tune[2] = voice[2].Coarse * 100 + voice[2].Fine;

	// Apply arpeggiator offset to the base pitch (affects all oscillators)
	int basePitch = currentPitch + (arpOffset * 100);
	
	// Apply global tuning (MainCoarse/MainFine from original monophonic version)
	int mtune = voiceMod->mainCoarse * 100 + voiceMod->mainFine;
	basePitch += mtune;
	
	// Apply main pitch modulation (affects all oscillators)
	basePitch += voiceMod->mainPitch;

	int opitch[3];
	opitch[0] = basePitch + tune[0] + voiceMod->oscPitch[0];
	opitch[1] = basePitch + tune[1] + voiceMod->oscPitch[1];
	opitch[2] = basePitch + tune[2] + voiceMod->oscPitch[2];

	// Set oscillator parameters
	for (int i = 0; i < 3; i++)
	{
		this->Oscs[i]->SetPitch(opitch[i]);
		// Apply pulse width modulation
		int pw = voice[i].Pw + voiceMod->oscPw[i];
		pw = (pw < 0) ? 0 : (pw > 65535) ? 65535 : pw;
		this->Oscs[i]->Set(pw, voice[i].Wave, voice[i].Sync);
	}

	// Render oscillators
	float o_val[3];
	for (int i = 0; i < 3; i++)
	{
		o_val[i] = this->Oscs[i]->Run();
	}

	// Process oscillator sync (must be called after Run())
	// Order matters: OSC2 syncs OSC1, OSC3 syncs OSC2, OSC1 syncs OSC3
	this->Oscs[1]->ProcessSync();
	this->Oscs[2]->ProcessSync();
	this->Oscs[0]->ProcessSync();

	// Mix oscillators with ring modulation
	float output = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		float oscOutput = o_val[i];

		// Ring modulation - multiply with next oscillator
		if (voice[i].Ring)
		{
			switch (i)
			{
			case 0:
				oscOutput *= o_val[1];  // OSC1 ring with OSC2
				break;
			case 1:
				oscOutput *= o_val[2];  // OSC2 ring with OSC3
				break;
			case 2:
				oscOutput *= o_val[0];  // OSC3 ring with OSC1
				break;
			}
		}

		// Apply oscillator volume with modulation
		float vol = voice[i].Volume + voiceMod->oscVol[i];
		vol = (vol < 0.0f) ? 0.0f : (vol > 5.0f) ? 5.0f : vol;
		oscOutput *= vol;

		// Hard clipping for individual oscillator (like original monophonic version)
		constexpr float clipThreshold = 1.0f;
		if (oscOutput > clipThreshold)
			oscOutput = clipThreshold;
		else if (oscOutput < -clipThreshold)
			oscOutput = -clipThreshold;

		output += oscOutput;
	}

	// CRITICAL: Normalize oscillator mix before filter (like original monophonic version)
	// 3 oscillators mixed → divide by 3 to maintain proper signal level
	output *= 0.333333f;

	// Run and store modulation envelope (for modulation matrix)
	modEnvValue = this->Envs[1]->Run();

	// Apply per-voice filter BEFORE envelope (correct signal chain: osc→filter→env)
	// This prevents envelope-induced signal variations from destabilizing high-Q filters
	switch (this->filterType)
	{
	default:
	case FTYPE_NONE:
		break;
	case FTYPE_DIRTY:
		output = this->FilterDirty->Run(output);
		if (!std::isfinite(output)) { this->FilterDirty->Reset(); output = 0.f; }
		break;
	case FTYPE_MOOG:
		output = this->FilterMoog->Run(output);
		if (!std::isfinite(output)) { this->FilterMoog->Reset(); output = 0.f; }
		break;
	case FTYPE_MOOG2:
		output = this->FilterMoog2->Run(output);
		if (!std::isfinite(output)) { this->FilterMoog2->Reset(); output = 0.f; }
		break;
	case FTYPE_CH12DB:
		output = this->FilterCh12db->Run(output);
		if (!std::isfinite(output)) { this->FilterCh12db->Reset(); output = 0.f; }
		break;
	case FTYPE_303:
		output = this->Filter303->Run(output);
		if (!std::isfinite(output)) { this->Filter303->Reset(); output = 0.f; }
		break;
	case FTYPE_8580:
		output = this->Filter8580->Run(output);
		if (!std::isfinite(output)) { this->Filter8580->Reset(); output = 0.f; }
		break;
	case FTYPE_BUDDA:
		output = this->FilterBiquad->Run(output);
		if (!std::isfinite(output)) { this->FilterBiquad->Reset(); output = 0.f; }
		break;
	}

	// Apply amplitude envelope AFTER filter
	float ampEnv = this->Envs[0]->Run();
	output *= ampEnv;

	// Check if voice should be deactivated (envelope finished)
	if (isReleasing && ampEnv <= 0.0001f)
	{
		isActive = false;
		isReleasing = false;
	}

	return output;
}

void CetoneSynthVoice::UpdateFilter(float cutoff, float q, float mod)
{
	// Update filter parameters (called from main synth when parameters change)
	switch (this->filterType)
	{
	default:
		break;
	case FTYPE_DIRTY:
		this->FilterDirty->Set(cutoff, q);
		break;
	case FTYPE_CH12DB:
		this->FilterCh12db->Set(cutoff, q);
		break;
	case FTYPE_MOOG:
		this->FilterMoog->Set(cutoff, q);
		break;
	case FTYPE_MOOG2:
		this->FilterMoog2->Set(cutoff, q);
		break;
	case FTYPE_303:
		this->Filter303->Set(cutoff, q, mod);
		break;
	case FTYPE_8580:
		this->Filter8580->Set(cutoff, q);
		break;
	case FTYPE_BUDDA:
		this->FilterBiquad->Set(cutoff, q);
		break;
	}
}

void CetoneSynthVoice::SetFilterType(int type)
{
	this->filterType = type;
}

void CetoneSynthVoice::SetFilterMode(int mode)
{
	// Apply mode to active filter and get actual mode used
	// (filters may not support all modes and will clamp to valid range)
	switch (this->filterType)
	{
	default:
		this->filterMode = 0;
		break;
	case FTYPE_DIRTY:
		this->FilterDirty->SetMode(mode);
		this->filterMode = this->FilterDirty->GetMode();
		break;
	case FTYPE_CH12DB:
		this->FilterCh12db->SetMode(mode);
		this->filterMode = this->FilterCh12db->GetMode();
		break;
	case FTYPE_MOOG:
		this->FilterMoog->SetMode(mode);
		this->filterMode = this->FilterMoog->GetMode();
		break;
	case FTYPE_MOOG2:
		this->FilterMoog2->SetMode(mode);
		this->filterMode = this->FilterMoog2->GetMode();
		break;
	case FTYPE_303:
		this->Filter303->SetMode(mode);
		this->filterMode = this->Filter303->GetMode();
		break;
	case FTYPE_8580:
		this->Filter8580->SetMode(mode);
		this->filterMode = this->Filter8580->GetMode();
		break;
	case FTYPE_BUDDA:
		this->filterMode = FMODE_LOW;
		break;
	}
}

int CetoneSynthVoice::GetFilterMode() const
{
	return this->filterMode;
}
