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

void CetoneSynthVoice::NoteOn(int note, int vel, bool portamento, int fromPitch)
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
		// Portamento will be handled in Render()
		doPorta = true;
		currentPitch = fromPitch;
		portaPitch = targetPitch;
		portaFrac = currentPitch << 14;
		// portaStep will be calculated in Render() based on portaSamples
	}
	else
	{
		doPorta = false;
		currentPitch = targetPitch;
		portaPitch = targetPitch;
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

float CetoneSynthVoice::Render(const SynthVoice voice[3], bool doPortamento, float portaSpeed, int portaSamples)
{
	if (!isActive)
		return 0.0f;

	// Handle portamento
	if (doPorta && doPortamento)
	{
		if (portaSamples > 0)
		{
			portaStep = (int)(((portaPitch - currentPitch) / (float)portaSamples) * 16384.0f + 0.5f);
		}

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

	int opitch[3];
	opitch[0] = currentPitch + tune[0];
	opitch[1] = currentPitch + tune[1];
	opitch[2] = currentPitch + tune[2];

	// Set oscillator parameters
	for (int i = 0; i < 3; i++)
	{
		this->Oscs[i]->SetPitch(opitch[i]);
		this->Oscs[i]->Set(voice[i].Pw, voice[i].Wave, voice[i].Sync);
	}

	// Render oscillators
	float o_val[3];
	for (int i = 0; i < 3; i++)
	{
		o_val[i] = this->Oscs[i]->Run();

		// Ring modulation
		if (voice[i].Ring)
		{
			int nextIdx = (i + 1) % 3;
			o_val[i] *= o_val[nextIdx];
		}
	}

	// Mix oscillators
	float output = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		output += o_val[i] * voice[i].Volume;
	}

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
