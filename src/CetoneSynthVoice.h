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

	// Note events
	void			NoteOn(int note, int velocity, bool portamento, int fromPitch);
	void			NoteOff();

	// Audio rendering
	float			Render(const SynthVoice voice[3], bool doPortamento, float portaSpeed, int portaSamples);

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
};
