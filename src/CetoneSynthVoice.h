#pragma once

#include "Structures.h"

// Forward declarations
class CSynthOscillator;
class CSynthEnvelope;
class CSynthLfo;

// Single polyphonic voice
class CetoneSynthVoice
{
public:
	CetoneSynthVoice();
	~CetoneSynthVoice();

	// Voice state
	bool			IsActive() const { return isActive; }
	bool			IsReleasing() const { return isReleasing; }
	int				GetNote() const { return noteNumber; }
	int				GetAge() const { return voiceAge; }
	float			GetEnvelopeLevel() const;
	float			GetModEnvelope() const { return modEnvValue; }	// Get modulation envelope value

	// Note events
	void			NoteOn(int note, int velocity, bool portamento, int fromPitch, int portaSamples);
	void			NoteOff();

	// Audio rendering
	float			Render(const SynthVoice voice[3], bool doPortamento, float portaSpeed, int portaSamples, 
						   const VoiceModulation* voiceMod, int arpOffset = 0);

	// Parameter updates
	void			UpdateEnvelopes(float attack0, float hold0, float decay0, float sustain0, float release0,
									float attack1, float hold1, float decay1, float sustain1, float release1);
	void			SetLfoParams(float speed, int pw, int wave, bool trigger);
	void			TriggerLfo();

	// Voice age management (for voice stealing)
	void			IncrementAge() { if (isActive) voiceAge++; }
	void			ResetAge() { voiceAge = 0; }

	// Reset voice to initial state
	void			Reset();

	// Arpeggiator support (for polyphonic arp mode)
	void			InitArpeggiator(int arpDelay);
	int				GetArpOffset(int arpMode, const int arpTable[8][16]);

private:
	// Audio components
	CSynthOscillator*	Oscs[3];
	CSynthEnvelope*		Envs[2];
	CSynthLfo*			Lfo;

	// Voice state
	bool				isActive;
	bool				isReleasing;
	int					noteNumber;
	int					velocity;
	int					voiceAge;

	// Pitch and portamento
	int					currentPitch;
	int					portaPitch;
	int					portaFrac;
	int					portaStep;
	bool				doPorta;

	// Pulse width for each oscillator
	int					voicePulsewidth[3];

	// Velocity modulation
	float				velocityMod;
	float				velocityModStep;
	float				velocityModEnd;

	// Modulation envelope value (updated each sample)
	float				modEnvValue;

	// Arpeggiator state (for polyphonic mode)
	int					arpPos;
	int					arpCounter;
	int					arpDelay;
};
