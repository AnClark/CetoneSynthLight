#include <math.h>

#include "CetoneSynth.h"
#include "CetoneSynthVoice.h"

void CCetoneSynth::process(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	this->SynthProcess(inputs, outputs, sampleFrames, false);
}

void CCetoneSynth::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	this->SynthProcess(inputs, outputs, sampleFrames, true);
}

void CCetoneSynth::SynthProcess(float **inputs, float **outputs, VstInt32 sampleFrames, bool replace)
{
	float* outl = outputs[0];
	float* outr = outputs[1];
	
	int p0, p1, p2, delta, tdelta = 0;
	float l, r;

	while(sampleFrames > 0)
	{
		if(this->CurrentDelta == 0)
		{
			while(!this->MidiStack->IsEmpty() && (this->CurrentDelta == 0))
			{
				this->MidiStack->Pop(&p0, &p1, &p2, &delta);
				
				if(delta != 0)
				{
					this->NextP0 = p0;
					this->NextP1 = p1;
					this->NextP2 = p2;
					this->CurrentDelta = delta;
				}
				else
				{
					this->HandleMidi(p0, p1, p2);
				}
			}
		}

		/****************************************************************************

		Run starts - Polyphonic Voice Mixing

		****************************************************************************/
		
		// Update global cutoff smoothing
		if(this->CutoffStep != 0.f)
		{
			this->Cutoff += this->CutoffStep;

			if(this->CutoffStep < 0.f)
			{
				if(this->Cutoff <= this->CutoffDest)
				{
					this->Cutoff		= this->CutoffDest;
					this->CutoffStep	= 0.f;
				}
			}
			else
			{
				if(this->Cutoff >= this->CutoffDest)
				{
					this->Cutoff		= this->CutoffDest;
					this->CutoffStep	= 0.f;
				}
			}
		}

		// Global parameters with modulation
		float m_vol = this->Volume;
		float m_cutoff = this->Cutoff;
		float m_q = this->Resonance;
		float m_mod = this->EnvMod;
		float m_pan = this->Panning;

		if(this->FilterCounter <= 0)
			this->FilterCounter = FILTER_DELAY;
		else
			this->FilterCounter--;

		// Update velocity and ctrl1 modulation smoothing
		if(this->Ctrl1ModStep != 0.f)
		{
			this->Ctrl1Mod += this->Ctrl1ModStep;

			if(this->Ctrl1ModStep < 0.f)
			{
				if(this->Ctrl1Mod <= this->Ctrl1ModEnd)
				{
					this->Ctrl1Mod = this->Ctrl1ModEnd;
					this->Ctrl1ModStep = 0.f;
				}
			}
			else
			{
				if(this->Ctrl1Mod >= this->Ctrl1ModEnd)
				{
					this->Ctrl1Mod = this->Ctrl1ModEnd;
					this->Ctrl1ModStep = 0.f;
				}
			}
		}

		if(this->VelocityModStep != 0.f)
		{
			this->VelocityMod += this->VelocityModStep;

			if(this->VelocityModStep < 0.f)
			{
				if(this->VelocityMod <= this->VelocityModEnd)
				{
					this->VelocityMod = this->VelocityModEnd;
					this->VelocityModStep = 0.f;
				}
			}
			else
			{
				if(this->VelocityMod >= this->VelocityModEnd)
				{
					this->VelocityMod = this->VelocityModEnd;
					this->VelocityModStep = 0.f;
				}
			}
		}

		// Run global LFO for modulation
		float mLfo = this->Lfo->Run();

		// Process global modulations (simplified for polyphony)
		// Note: For full per-voice modulation, these would need to be inside each voice
		for(int i = 0; i < 4; i++)
		{
			float am;
			SynthModulation* mod = &(this->Modulations[i]);

			if (mod->Source == MOD_SRC_NONE)
				continue;

			switch(mod->Source)
			{
			case MOD_SRC_VEL:			am = this->VelocityMod;		break;
			case MOD_SRC_CTRL1:			am = this->Ctrl1Mod;		break;
			case MOD_SRC_LFO1:			am = mLfo;					break;
			default:					am = 0.f;					break;
			}

			am *= mod->Amount;
			am *= mod->Multiplicator;

			switch(mod->Destination)
			{
			case MOD_DEST_MAINVOL:
				m_vol += am * 0.0005f;
				m_vol = (m_vol < 0.f) ? 0.f : (m_vol > 5.f) ? 5.f : m_vol;
				break;
			case MOD_DEST_CUTOFF:
				m_cutoff += am * 0.0001f;
				m_cutoff = (m_cutoff < 0.f) ? 0.f : (m_cutoff > 1.f) ? 1.f : m_cutoff;
				break;
			case MOD_DEST_RESONANCE:
				m_q += am * 0.0001f;
				m_q = (m_q < 0.f) ? 0.f : (m_q > 1.f) ? 1.f : m_q;
				break;
			case MOD_DEST_PANNING:
				m_pan += am * 0.0001f;
				m_pan = (m_pan < 0.f) ? 0.f : (m_pan > 1.f) ? 1.f : m_pan;
				break;
			case MOD_DEST_ENVMOD:
				m_mod += am * 0.0001f;
				m_mod = (m_mod < -1.f) ? -1.f : (m_mod > 1.f) ? 1.f : m_mod;
				break;
			default:
				break;
			}
		}

		// Update filters with modulated parameters
		this->UpdateFilters(m_cutoff, m_q, m_mod);

		// Process arpeggiator (mono mode only - shared across all voices)
		int arpOffset = 0;  // Semitone offset from arpeggiator
		bool monoArpActive = (this->ArpMode != -1) && !this->ArpPoly;
		
		if (monoArpActive)
		{
			// Check if arpeggiator position needs to wrap
			if (this->ArpPos >= this->C64Arps[this->ArpMode][15])
				this->ArpPos = 0;

			// Get current arpeggio offset in semitones
			arpOffset = this->C64Arps[this->ArpMode][this->ArpPos];

			// Update arpeggiator counter
			this->ArpCounter--;
			if (this->ArpCounter <= 0)
			{
				this->ArpCounter = this->ArpDelay;
				this->ArpPos++;
			}
		}

		// Mix all active voices
		float output = 0.f;
		int activeCount = 0;

		// Determine which note to render in mono arpeggiator mode
		int arpNote = this->CurrentNote;
		bool monoArpNoteCheck = monoArpActive && (this->CurrentNote != -1);

		for (int v = 0; v < this->maxPolyphony; v++)
		{
			if (this->Voices[v]->IsActive())
			{
				// Calculate arpeggio offset for this voice
				int voiceArpOffset = 0;
				
				if (this->ArpPoly && this->ArpMode != -1)
				{
					// Polyphonic arpeggiator: each voice calculates its own offset
					voiceArpOffset = this->Voices[v]->GetArpOffset(this->ArpMode, this->C64Arps);
				}
				else if (monoArpActive)
				{
					// Monophonic arpeggiator: only CurrentNote voice gets the offset
					if (this->Voices[v]->GetNote() == arpNote)
						voiceArpOffset = arpOffset;
					else
						continue;  // Skip other voices in mono arp mode
				}

				float voiceOutput = this->Voices[v]->Render(
					this->Voice,
					this->PortaMode,
					this->PortaSpeed,
					(int)this->PortaSamples,
					voiceArpOffset
				);
				output += voiceOutput;
				activeCount++;
			}
		}

		// Normalize to prevent clipping when multiple voices are playing
		// Use fixed normalization factor to avoid volume jumps when voice count changes
		// Factor chosen to balance single-voice volume with polyphonic headroom
		if (activeCount > 0)
		{
			// Divide by ~4.5 provides good balance:
			// - Single voice has decent volume (comparable to original)
			// - Multiple voices have headroom before clipping
			output *= 0.22f;  // Approximately 1/4.5
		}

		// Apply global filter
		switch(this->FilterType)
		{
		default:
			break;
		case FTYPE_DIRTY:
			output = this->FilterDirty->Run(output);
			break;
		case FTYPE_MOOG:
			output = this->FilterMoog->Run(output);
			break;
		case FTYPE_MOOG2:
			output = this->FilterMoog2->Run(output);
			break;
		case FTYPE_CH12DB:
			output = this->FilterCh12db->Run(output);
			break;
		case FTYPE_303:
			output = this->Filter303->Run(output);
			break;
		case FTYPE_8580:
			output = this->Filter8580->Run(output);
			break;
		case FTYPE_BUDDA:
			output = this->FilterBiquad->Run(output);
			break;
		}

		// Apply global volume and panning
		output *= m_vol;

		l = ((1.f - m_pan) * output);
		r = (m_pan * output);

		/****************************************************************************

		Run ends

		****************************************************************************/

		if (replace)
		{
			(*outl++) = l;
			(*outr++) = r;
		}
		else
		{
			(*outl++) += l;
			(*outr++) += r;
		}
		
		sampleFrames--;
		tdelta++;

		if(this->CurrentDelta > 0)
		{
			if(tdelta >= this->CurrentDelta)
			{
				this->CurrentDelta = 0;
				this->HandleMidi(this->NextP0, this->NextP1, this->NextP2);
			}
		}
	}
}

VstInt32 CCetoneSynth::processEvents(const DISTRHO::MidiEvent *events, uint32_t eventCount)
{
	for (VstInt32 i = 0; i < eventCount; i++)
	{
		const DISTRHO::MidiEvent &_event = events[i];
		
		auto midiData = _event.data;
		this->MidiStack->Push(midiData[0], midiData[1] & 0x7f, midiData[2] & 0x7f, _event.frame);
	}
	return 1;
}

void CCetoneSynth::HandleMidi(int p0, int p1, int p2)
{
	bool btmp;
	int status = p0 & 0xf0;

	switch (status)
	{
	case 0x80:			// Note off
		this->NoteOff(p1, p2);
		break;
	case 0x90:			// Note on
		if (p2 == 0)
			this->NoteOff(p1, 0);
		else
			this->NoteOn(p1, p2);
		break;
	case 0xb0:			// Control change
		switch (p1)
		{
		case 1:			// Control #1
			this->CurrentCtrl1	= p2;
			this->Ctrl1ModEnd	= (float)(p2 - 64) / 64.f;
			if(this->Ctrl1ModEnd != this->Ctrl1Mod)
			{
				this->Ctrl1ModStep = (this->Ctrl1ModEnd - this->Ctrl1Mod) * this->ModChangeSamples;
			}
			else
				this->Ctrl1ModStep = 0.f;
			break;
		case 5:			// Portamento time
			this->setParameterAutomated(pPortaSpeed, (float)p2 / 127.f);
			break;
		case 7:			// Volume
			this->setParameterAutomated(pVolume, (float)p2 / 127.f);
			break;
		case 8:			// Panning
			this->setParameterAutomated(pPanning, (float)p2 / 127.f);
			break;
		case 65:		// Portamento switch
			btmp = (p2 == 0) ? false : true;
			if(!btmp && this->DoPorta)
			{
				this->CurrentPitch	=	this->PortaPitch;
				this->DoPorta		=	false;
			}
			this->setParameterAutomated(pPortaMode, (btmp) ? 1.f : 0.f);
			break;
		case 75:		// Cutoff
			this->setParameterAutomated(pCutoff, (float)p2 / 127.f);
			break;
		case 76:		// Resonance
			this->setParameterAutomated(pResonance, (float)p2 / 127.f);
			break;
		case 80:		// Mod 1 Amount
			this->setParameterAutomated(pMod1Amount, (float)p2 / 127.f);
			break;
		case 81:		// Mod 2 Amount
			this->setParameterAutomated(pMod2Amount, (float)p2 / 127.f);
			break;
		case 82:		// Mod 3 Amount
			this->setParameterAutomated(pMod3Amount, (float)p2 / 127.f);
			break;
		case 83:		// Mod 4 Amount
			this->setParameterAutomated(pMod4Amount, (float)p2 / 127.f);
			break;
		case 120:		// All Sounds Off (MIDI panic)
		case 123:		// All Notes Off (MIDI panic)
			this->Panic();
			break;
		}
		break;
	case 0xc0:			// Program change
		this->ReadProgram(p1);
		break;
	}
}

void CCetoneSynth::NoteOn(int note, int vel)
{
	// Allocate a voice for this note
	int voiceIndex = this->AllocateVoice(note);
	if (voiceIndex < 0)
		return; // Failed to allocate (shouldn't happen)

	// Calculate target pitch for new note
	int targetPitch = (note + NOTE_OFFSET) * 100;
	
	// Determine if we should use portamento
	// Original behavior: portamento if there was a previous note (CurrentNote != -1)
	// regardless of whether that note is still playing
	bool usePorta = (this->PortaMode && (this->PortaSpeed != 0.f) && (this->CurrentNote != -1));
	int fromPitch = usePorta ? this->CurrentPitch : targetPitch;

	// Update current note tracking (for portamento reference)
	this->CurrentNote = note;
	this->CurrentVelocity = vel;
	
	// Always update CurrentPitch to target (for next note's portamento reference)
	// The voice will handle sliding from fromPitch to CurrentPitch if portamento is active
	this->CurrentPitch = targetPitch;

	// Update velocity modulation (global)
	this->VelocityModEnd = (float)vel / 127.f;
	if (this->VelocityModEnd != this->VelocityMod)
	{
		this->VelocityModStep = (this->VelocityModEnd - this->VelocityMod) * this->ModChangeSamples;
	}
	else
		this->VelocityModStep = 0.f;

	// Trigger the voice
	this->Voices[voiceIndex]->NoteOn(note, vel, usePorta, fromPitch, (int)this->PortaSamples);
	this->Voices[voiceIndex]->UpdateEnvelopes(
		this->EnvAttack[0], this->EnvHold[0], this->EnvDecay[0], this->EnvSustain[0], this->EnvRelease[0],
		this->EnvAttack[1], this->EnvHold[1], this->EnvDecay[1], this->EnvSustain[1], this->EnvRelease[1]
	);
	this->Voices[voiceIndex]->SetLfoParams(this->LfoSpeed, this->LfoPw, this->LfoWave, this->LfoTrigger);
	this->Voices[voiceIndex]->TriggerLfo();

	// Initialize arpeggiator for this voice (polyphonic mode)
	if (this->ArpPoly && this->ArpMode != -1)
	{
		this->Voices[voiceIndex]->InitArpeggiator(this->ArpDelay);
	}

	// Increment age for all other active voices (for voice stealing)
	for (int i = 0; i < this->maxPolyphony; i++)
	{
		if (i != voiceIndex)
			this->Voices[i]->IncrementAge();
	}
}

void CCetoneSynth::NoteOff(int note, int vel)
{
	// Find all voices playing this note and release them
	for (int i = 0; i < this->maxPolyphony; i++)
	{
		if (this->Voices[i]->IsActive() && this->Voices[i]->GetNote() == note)
		{
			this->Voices[i]->NoteOff();
		}
	}
	
	// Do NOT clear CurrentNote - it should be preserved for portamento
	// Original monophonic behavior: NoteOff only releases envelopes,
	// CurrentNote stays for next note's portamento reference
}

void CCetoneSynth::Panic()
{
	// MIDI panic - immediately stop all voices
	for (int i = 0; i < this->maxPolyphony; i++)
	{
		// Use Reset() instead of NoteOff() to immediately silence the voice
		// This is the correct behavior for ALL_SOUNDS_OFF and ALL_NOTES_OFF
		this->Voices[i]->Reset();
	}

	// Reset current note tracking and voice management state
	this->CurrentNote = -1;
	this->activeVoiceCount = 0;
	
	// Reset portamento state
	this->DoPorta = false;
	this->CurrentPitch = 0;
	this->PortaPitch = 0;
}

void CCetoneSynth::UpdateFilters()
{
	this->UpdateFilters(this->Cutoff, this->Resonance, this->EnvMod);
}

void CCetoneSynth::UpdateFilters(float cutoff, float q, float mod)
{
	if(this->FilterCounter != FILTER_DELAY)
		return;

	switch (this->FilterType)
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

void CCetoneSynth::SetFilterMode(int mode)
{
	switch (this->FilterType)
	{
	default:
		mode = 0;
		break;
	case FTYPE_DIRTY:
		this->FilterDirty->SetMode(mode);
		mode = this->FilterDirty->GetMode();
		break;
	case FTYPE_CH12DB:
		this->FilterCh12db->SetMode(mode);
		mode = this->FilterCh12db->GetMode();
		break;
	case FTYPE_MOOG:
		this->FilterMoog->SetMode(mode);
		mode = this->FilterMoog->GetMode();
		break;
	case FTYPE_MOOG2:
		this->FilterMoog2->SetMode(mode);
		mode = this->FilterMoog2->GetMode();
		break;
	case FTYPE_303:
		this->Filter303->SetMode(mode);
		mode = this->Filter303->GetMode();
		break;
	case FTYPE_8580:
		this->Filter8580->SetMode(mode);
		mode = this->Filter8580->GetMode();
		break;
	case FTYPE_BUDDA:
		mode = FMODE_LOW;
		break;
	}

	this->FilterMode = mode;
	this->Programs[this->CurrentProgram].FilterMode = mode;
}

void CCetoneSynth::SetArpSpeed(int ms)
{
	float sec = (float)ms / 1000.f;
	this->ArpDelay = (int)(sec * this->SampleRate + 0.5f);
}

void CCetoneSynth::SetPortaSpeed(float speed)
{
	this->PortaSamples = floorf(speed * this->SampleRate + 0.5f);
}

void CCetoneSynth::UpdateEnvelopes()
{
	// Update all voices with new envelope parameters
	for (int v = 0; v < MAX_POLYPHONY; v++)
	{
		this->Voices[v]->UpdateEnvelopes(
			this->EnvAttack[0], this->EnvHold[0], this->EnvDecay[0], this->EnvSustain[0], this->EnvRelease[0],
			this->EnvAttack[1], this->EnvHold[1], this->EnvDecay[1], this->EnvSustain[1], this->EnvRelease[1]
		);
	}
}

void CCetoneSynth::resume()		// TODO: Call this in corresponding DPF API
{
	//AudioEffectX::resume();

#if ANALOGUE_BEHAVIOR == 0
	for (int v = 0; v < MAX_POLYPHONY; v++)
		this->Voices[v]->Reset();

	this->Lfo->Reset();
#endif
	
	// Reset all voices
	for (int v = 0; v < MAX_POLYPHONY; v++)
		this->Voices[v]->Reset();

	this->Filter303->Reset();
	this->Filter8580->Reset();
	this->FilterMoog->Reset();
	this->FilterMoog2->Reset();
	this->FilterDirty->Reset();
	this->FilterCh12db->Reset();

	this->VelocityMod = 0.f;
	this->Ctrl1Mod = 0.f;
	this->VelocityModStep = 0.f;
	this->Ctrl1ModStep = 0.f;

	this->FilterCounter = FILTER_DELAY;
	this->UpdateFilters();

	this->CutoffStep = 0.f;

	for(int i = 0; i < 1000; i++)
	{
		float output;
		
		switch(this->FilterType)
		{
		default:
			break;
		case FTYPE_DIRTY:
			output = this->FilterDirty->Run(0.f);
			break;
		case FTYPE_MOOG:
			output = this->FilterMoog->Run(0.f);
			break;
		case FTYPE_MOOG2:
			output = this->FilterMoog2->Run(0.f);
			break;
		case FTYPE_CH12DB:
			output = this->FilterCh12db->Run(0.f);
			break;
		case FTYPE_303:
			output = this->Filter303->Run(0.f);
			break;
		case FTYPE_8580:
			output = this->Filter8580->Run(0.f);
			break;
		case FTYPE_BUDDA:
			output = this->FilterBiquad->Run(0.f);
			break;
		}
	}
}

void CCetoneSynth::SetCutoffSave(float value)
{
	if(value != this->Cutoff)
	{
		this->CutoffDest = value;

		float delta = this->Cutoff - value;
		float samples = fabs(delta) * 10.f * this->ModChangeSamples;
		this->CutoffStep = delta / samples;
	}
	else
		this->CutoffStep = 0.f;
}
