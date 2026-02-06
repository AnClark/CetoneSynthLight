#include "CetoneSynthVoice.h"
#include "SynthOscillator.h"
#include "SynthEnvelope.h"
#include "SynthLfo.h"
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

	Reset();
}

CetoneSynthVoice::~CetoneSynthVoice()
{
	for (int i = 0; i < 3; i++)
		delete this->Oscs[i];

	for (int i = 0; i < 2; i++)
		delete this->Envs[i];

	delete this->Lfo;
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
		output += oscOutput * vol;
	}

	// Run and store modulation envelope (for modulation matrix)
	modEnvValue = this->Envs[1]->Run();

	// Apply amplitude envelope
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
