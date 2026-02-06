#include "CetoneSynth.h"
#include "CetoneSynthVoice.h"
#include "Defines.h"

// Allocate a voice for a new note
// Returns the voice index, or -1 if failed
int CCetoneSynth::AllocateVoice(int note)
{
	// First, try to find an inactive voice
	for (int i = 0; i < MAX_POLYPHONY; i++)
	{
		if (!this->Voices[i]->IsActive())
		{
			return i;
		}
	}

	// All voices are active, need to steal one
	// Strategy: Steal the oldest releasing voice, or the oldest active voice

	int oldestReleasingVoice = -1;
	int oldestReleasingAge = -1;

	int oldestActiveVoice = -1;
	int oldestActiveAge = -1;

	for (int i = 0; i < MAX_POLYPHONY; i++)
	{
		if (this->Voices[i]->IsReleasing())
		{
			int age = this->Voices[i]->GetAge();
			if (age > oldestReleasingAge)
			{
				oldestReleasingAge = age;
				oldestReleasingVoice = i;
			}
		}
		else if (this->Voices[i]->IsActive())
		{
			int age = this->Voices[i]->GetAge();
			if (age > oldestActiveAge)
			{
				oldestActiveAge = age;
				oldestActiveVoice = i;
			}
		}
	}

	// Prefer stealing releasing voices
	if (oldestReleasingVoice >= 0)
		return oldestReleasingVoice;

	// Otherwise steal the oldest active voice
	if (oldestActiveVoice >= 0)
		return oldestActiveVoice;

	// Fallback (should never happen)
	return 0;
}

// Find a voice playing a specific note
// Returns the voice index, or -1 if not found
int CCetoneSynth::FindVoiceByNote(int note)
{
	for (int i = 0; i < MAX_POLYPHONY; i++)
	{
		if (this->Voices[i]->IsActive() && this->Voices[i]->GetNote() == note)
		{
			return i;
		}
	}
	return -1;
}

// Update all voice parameters from current synth settings
void CCetoneSynth::UpdateAllVoices()
{
	for (int i = 0; i < MAX_POLYPHONY; i++)
	{
		this->Voices[i]->UpdateEnvelopes(
			this->EnvAttack[0], this->EnvHold[0], this->EnvDecay[0], this->EnvSustain[0], this->EnvRelease[0],
			this->EnvAttack[1], this->EnvHold[1], this->EnvDecay[1], this->EnvSustain[1], this->EnvRelease[1]
		);

		this->Voices[i]->SetLfoParams(this->LfoSpeed, this->LfoPw, this->LfoWave, this->LfoTrigger);
	}
}
