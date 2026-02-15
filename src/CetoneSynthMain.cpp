#include <math.h>

#include "CetoneSynth.h"
#ifdef ENABLE_POLYPHONY
#include "CetoneSynthVoice.h"
#endif

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

#ifndef ENABLE_POLYPHONY
	int opitch[3];
	int opw[3];
	float output;
	float l_speed;
	float o_val[3];
	float v_vol[3], mEnv, mLfo, mMix;

	int tune[4], mtune;

	tune[0] = this->Voice[0].Coarse * 100 + this->Voice[0].Fine;
	tune[1] = this->Voice[1].Coarse * 100 + this->Voice[1].Fine;
	tune[2] = this->Voice[2].Coarse * 100 + this->Voice[2].Fine;
#endif

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

#ifdef ENABLE_POLYPHONY
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

		// Get modulation envelope from current note's voice
		// In monophonic original, there was one envelope; in polyphonic, use CurrentNote's voice
		float mEnv = 0.0f;
		if (this->CurrentNote != -1)
		{
			// Find voice playing current note
			for (int v = 0; v < this->maxPolyphony; v++)
			{
				if (this->Voices[v]->IsActive() && this->Voices[v]->GetNote() == this->CurrentNote)
				{
					mEnv = this->Voices[v]->GetModEnvelope();
					break;
				}
			}
		}
		float mMix = mEnv * mLfo;

		// Initialize voice modulation structure
		VoiceModulation voiceMod;
		voiceMod.mainPitch = 0;
		voiceMod.mainCoarse = this->MainCoarse;
		voiceMod.mainFine = this->MainFine;
		voiceMod.oscPitch[0] = voiceMod.oscPitch[1] = voiceMod.oscPitch[2] = 0;
		voiceMod.oscVol[0] = voiceMod.oscVol[1] = voiceMod.oscVol[2] = 0.0f;
		voiceMod.oscPw[0] = voiceMod.oscPw[1] = voiceMod.oscPw[2] = 0;
		voiceMod.lfoSpeed = 0.0f;

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
			case MOD_SRC_MENV1:			am = mEnv;					break;
			case MOD_SRC_LFO1:			am = mLfo;					break;
			case MOD_SRC_MENV1xLFO1:	am = mMix;					break;
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
			case MOD_DEST_MAINPITCH:
				voiceMod.mainPitch += (int)am;
				break;
			case MOD_DEST_OSC1VOL:
				voiceMod.oscVol[0] += am * 0.0005f;
				break;
			case MOD_DEST_OSC2VOL:
				voiceMod.oscVol[1] += am * 0.0005f;
				break;
			case MOD_DEST_OSC3VOL:
				voiceMod.oscVol[2] += am * 0.0005f;
				break;
			case MOD_DEST_OSC1PITCH:
				voiceMod.oscPitch[0] += (int)am;
				break;
			case MOD_DEST_OSC2PITCH:
				voiceMod.oscPitch[1] += (int)am;
				break;
			case MOD_DEST_OSC3PITCH:
				voiceMod.oscPitch[2] += (int)am;
				break;
			case MOD_DEST_OSC1PW:
				voiceMod.oscPw[0] += (int)(am * 6.5536f);
				break;
			case MOD_DEST_OSC2PW:
				voiceMod.oscPw[1] += (int)(am * 6.5536f);
				break;
			case MOD_DEST_OSC3PW:
				voiceMod.oscPw[2] += (int)(am * 6.5536f);
				break;
			case MOD_DEST_LFO1SPEED:
				voiceMod.lfoSpeed += am * 0.005f;
				break;
			default:
				break;
			}
		}

		// Apply LFO speed modulation to global LFO
		if (voiceMod.lfoSpeed != 0.0f)
		{
			float modulatedSpeed = this->LfoSpeed + voiceMod.lfoSpeed;
			modulatedSpeed = (modulatedSpeed < 0.0f) ? 0.0f : modulatedSpeed;
			this->Lfo->SetSpeed(modulatedSpeed);
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
					&voiceMod,
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
			// Per-voice filter application maintains consistent signal level, so we can use a wider normalization factor.
			// This factor is as same as the monophonic version (who mixes 3 OSCs together)
			output *= 0.333333f;  // Approximately 1/3
		}

		// NOTE: Filter is now applied per-voice in Voice::Render()
		// This maintains correct signal chain: oscillators → filter → envelope
		// Prevents envelope-induced variations from destabilizing high-Q filters

		// Apply global volume and panning
		output *= m_vol;

		l = ((1.f - m_pan) * output);
		r = (m_pan * output);

		/****************************************************************************

		Run ends

		****************************************************************************/
#else
		/****************************************************************************

		Run starts

		****************************************************************************/
		
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

		int m_coarse = this->MainCoarse;
		int m_fine = this->MainFine, itmp;

		float m_vol = this->Volume;
		float m_cutoff = this->Cutoff;
		float m_q = this->Resonance;
		float m_mod = this->EnvMod;
		float m_pan = this->Panning;

		bool set_opw0 = false;
		bool set_opw1 = false;
		bool set_opw2 = false;
		bool set_lfo0 = false;

		if(this->FilterCounter <= 0)
			this->FilterCounter = FILTER_DELAY;
		else
			this->FilterCounter--;

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

		if(this->CurrentNote == -1)
		{
			l = r = 0.f;
		}
		else
		{
			opw[0]		= this->VoicePulsewidth[0];
			opw[1]		= this->VoicePulsewidth[1];
			opw[2]		= this->VoicePulsewidth[2];

			v_vol[0]	= this->Voice[0].Volume;
			v_vol[1]	= this->Voice[1].Volume;
			v_vol[2]	= this->Voice[2].Volume;

			l_speed		= this->LfoSpeed;

			if (this->DoPorta)
			{
				this->PortaFrac += this->PortaStep;
				itmp = this->PortaFrac >> 14;

				if(this->PortaStep < 0)
				{
					if(itmp <= this->PortaPitch)
					{
						itmp = this->PortaPitch;
						this->DoPorta = false;
					}
				}
				else
				{
					if(itmp >= this->PortaPitch)
					{
						itmp = this->PortaPitch;
						this->DoPorta = false;
					}
				}

				this->CurrentPitch = itmp;
			}

			opitch[0] = this->CurrentPitch + tune[0];
			opitch[1] = this->CurrentPitch + tune[1];
			opitch[2] = this->CurrentPitch + tune[2];

			mEnv = this->Envs[1]->Run();
			mLfo = this->Lfo->Run();
			mMix = mEnv * mLfo;

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
				case MOD_SRC_MENV1:			am = mEnv;				break;
				case MOD_SRC_LFO1:			am = mLfo;				break;
				case MOD_SRC_MENV1xLFO1:	am = mMix;				break;
				}

				am *= mod->Amount;
				am *= mod->Multiplicator;

				switch(mod->Destination)
				{
				default:
					break;
				case MOD_DEST_MAINVOL:
					m_vol += am * 0.0005f;
					if(m_vol < 0.f)	
						m_vol = 0.f;	
					else if(m_vol > 5.f)	
						m_vol = 5.f;
					break;
				case MOD_DEST_CUTOFF:
					m_cutoff += am * 0.0001f;
					if(m_cutoff < 0.f) 
						m_cutoff = 0.f;	
					else if(m_cutoff > 1.f)	
						m_cutoff = 1.f;
					break;
				case MOD_DEST_RESONANCE:
					m_q += am * 0.0001f;
					if(m_q < 0.f)	
						m_q = 0.f;	
					else if(m_q > 1.f)	
						m_q = 1.f;
					break;
				case MOD_DEST_PANNING:
					m_pan += am * 0.0001f;
					if(m_pan < 0.f)	
						m_pan = 0.f; 
					else if(m_pan > 1.f)	
						m_pan = 1.f;
					break;
				case MOD_DEST_MAINPITCH:
					m_fine += truncate(am);
					break;
				case MOD_DEST_OSC1VOL:
					v_vol[0] += am * 0.0005f;
					if(v_vol[0] < 0.f)	
						v_vol[0] = 0.f;	
					else if(v_vol[0] > 5.f)	
						v_vol[0] = 5.f;
					break;
				case MOD_DEST_OSC2VOL:
					v_vol[1] += am * 0.0005f;
					if(v_vol[1] < 0.f)	
						v_vol[1] = 0.f;	
					else if(v_vol[1] > 5.f)	
						v_vol[1] = 5.f;
					break;
				case MOD_DEST_OSC3VOL:
					v_vol[2] += am * 0.0005f;
					if(v_vol[2] < 0.f)	
						v_vol[2] = 0.f;	
					else if(v_vol[2] > 5.f)	
						v_vol[2] = 5.f;
					break;
				case MOD_DEST_OSC1PITCH:
					opitch[0] += truncate(am);
					break;
				case MOD_DEST_OSC2PITCH:
					opitch[1] += truncate(am);
					break;
				case MOD_DEST_OSC3PITCH:
					opitch[2] += truncate(am);
					break;
				case MOD_DEST_OSC1PW:
					opw[0] += truncate(am * 6.5536f);
					set_opw0 = true;
					break;
				case MOD_DEST_OSC2PW:
					opw[1] += truncate(am * 6.5536f);
					set_opw1 = true;
					break;
				case MOD_DEST_OSC3PW:
					opw[2] += truncate(am * 6.5536f);
					set_opw2 = true;
					break;
				case MOD_DEST_LFO1SPEED:
					l_speed += am * 0.005f;
					set_lfo0 = true;
					break;
				case MOD_DEST_ENVMOD:
					m_mod += am * 0.0001f;
					if(m_mod < -1.f)	
						m_mod = -1.f; 
					else if(m_mod > 1.f)	
						m_mod = 1.f;
					break;
				}
			}

			if(set_opw0)
				this->Oscs[0]->SetPw(opw[0]);
			if(set_opw1)
				this->Oscs[1]->SetPw(opw[1]);
			if(set_opw2)
				this->Oscs[2]->SetPw(opw[2]);
			if(set_lfo0)
				this->Lfo->SetSpeed(l_speed);

			if(this->ArpMode != -1)
			{
				if(this->ArpPos >= this->C64Arps[this->ArpMode][15])
					this->ArpPos = 0;

				m_coarse += this->C64Arps[this->ArpMode][this->ArpPos];

				this->ArpCounter--;

				if(this->ArpCounter <= 0)
				{
					this->ArpCounter = this->ArpDelay;
					this->ArpPos++;
				}
			}

			mtune = m_coarse * 100 + m_fine;

			output = 0.f;

			float output_volume = this->Envs[0]->Run() * m_vol;

			this->UpdateFilters(m_cutoff, m_q, m_mod);
			
			for(int i = 0; i < 3; i++)
			{
				opitch[i]	+= mtune;
				this->Oscs[i]->SetPitch(opitch[i]);
				o_val[i]	= this->Oscs[i]->Run();
			}

			for(int i = 0; i < 3; i++)
			{
				float ftmp = o_val[i];

				if(this->Voice[i].Ring)
				{
					switch(i)
					{
						case 0:
							ftmp *= o_val[1];
							break;
						case 1:
							ftmp *= o_val[2];
							break;
						case 2:
							ftmp *= o_val[0];
							break;
					}
				}
				
				ftmp = ftmp * v_vol[i];

				if(ftmp > 1.f)
					ftmp = 1.f;
				else if(ftmp < -1.f)
					ftmp = -1.f;

				output += ftmp;
			}

			this->Oscs[1]->ProcessSync();
			this->Oscs[2]->ProcessSync();
			this->Oscs[0]->ProcessSync();

			output *= 0.333333f;

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

			output *= output_volume;

			l = ((1.f - m_pan) * output);
			r = (m_pan * output);
		}

		/****************************************************************************

		Run ends

		****************************************************************************/
#endif

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
#ifdef ENABLE_POLYPHONY
		this->NoteOff(p1, p2);
#else
		if (p1 == this->CurrentNote)
			this->NoteOff(p1, p2);
#endif
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
#ifdef ENABLE_POLYPHONY
		case 120:		// All Sounds Off (MIDI panic)
		case 123:		// All Notes Off (MIDI panic)
			this->Panic();
			break;
#else
		case 123:		// All Notes Off
			this->CurrentNote	= -1;
			break;
#endif
		}
		break;
	case 0xc0:			// Program change
		this->ReadProgram(p1);
		break;
	}
}

void CCetoneSynth::NoteOn(int note, int vel)
{
#ifdef ENABLE_POLYPHONY
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
#else
	int tmp;
	bool porta = (this->PortaMode && (this->PortaSpeed != 0.f) && (this->CurrentNote != -1)) ? true : false;

	this->CurrentNote		= note;
	this->CurrentVelocity	= vel;
	this->VelocityModEnd	= (float)vel / 127.f;

	if(this->VelocityModEnd != this->VelocityMod)
	{
		this->VelocityModStep = (this->VelocityModEnd - this->VelocityMod) * this->ModChangeSamples;
	}
	else
		this->VelocityModStep = 0.f;

	tmp = (note + NOTE_OFFSET) * 100;

	if(porta)
	{
		this->PortaStep		= (int)(((tmp - this->CurrentPitch) / (this->PortaSamples)) * 16384.f + 0.5f);
		this->PortaFrac     = this->CurrentPitch << 14;
		this->PortaPitch	= tmp;
	}
	else
	{
		this->CurrentPitch = tmp;
	}

	for(int i = 0; i < 3; i++)
	{
		this->VoicePulsewidth[i] = this->Voice[i].Pw;
		this->Oscs[i]->Set(this->VoicePulsewidth[i], this->Voice[i].Wave, this->Voice[i].Sync);
	}

	this->DoPorta		= porta;

	this->Envs[0]->Gate(true);
	this->Envs[1]->Gate(true);

	this->Lfo->Set(this->LfoSpeed, this->LfoPw, this->LfoWave, this->LfoTrigger);
	this->Lfo->Trigger();
#endif
}

void CCetoneSynth::NoteOff(int note, int vel)
{
#ifdef ENABLE_POLYPHONY
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
#else
	this->Envs[0]->Gate(false);
	this->Envs[1]->Gate(false);
#endif
}

#ifdef ENABLE_POLYPHONY
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
#endif

void CCetoneSynth::UpdateFilters()
{
	this->UpdateFilters(this->Cutoff, this->Resonance, this->EnvMod);
}

void CCetoneSynth::UpdateFilters(float cutoff, float q, float mod)
{
	if(this->FilterCounter != FILTER_DELAY)
		return;

#ifdef ENABLE_POLYPHONY
	// In polyphonic mode, update per-voice filters
	for (int v = 0; v < MAX_POLYPHONY; v++)
	{
		this->Voices[v]->UpdateFilter(cutoff, q, mod);
	}
#else
	// In monophonic mode, update global filter
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
#endif
}

void CCetoneSynth::SetFilterMode(int mode)
{
#ifdef ENABLE_POLYPHONY
	// In polyphonic mode, update per-voice filters
	for (int v = 0; v < MAX_POLYPHONY; v++)
	{
		this->Voices[v]->SetFilterMode(mode);
	}
	// Store mode based on filter type capabilities
	switch (this->FilterType)
	{
	default:
		mode = 0;
		break;
	case FTYPE_BUDDA:
		mode = FMODE_LOW;
		break;
	}
#else
	// In monophonic mode, update global filter
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
#endif

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
#ifdef ENABLE_POLYPHONY
	// Update all voices with new envelope parameters
	for (int v = 0; v < MAX_POLYPHONY; v++)
	{
		this->Voices[v]->UpdateEnvelopes(
			this->EnvAttack[0], this->EnvHold[0], this->EnvDecay[0], this->EnvSustain[0], this->EnvRelease[0],
			this->EnvAttack[1], this->EnvHold[1], this->EnvDecay[1], this->EnvSustain[1], this->EnvRelease[1]
		);
	}
#else
	for(int i = 0; i < 2; i++)
		this->Envs[i]->Set(this->EnvAttack[i], this->EnvHold[i], this->EnvDecay[i], this->EnvSustain[i], this->EnvRelease[i]);
#endif
}

void CCetoneSynth::resume()		// TODO: Call this in corresponding DPF API
{
	//AudioEffectX::resume();

#if ANALOGUE_BEHAVIOR == 0
#ifdef ENABLE_POLYPHONY
	for (int v = 0; v < MAX_POLYPHONY; v++)
		this->Voices[v]->Reset();

	this->Lfo->Reset();
#else
	this->Oscs[0]->Reset();
	this->Oscs[1]->Reset();
	this->Oscs[2]->Reset();

	this->Lfos->Reset();
#endif	// ENABLE_POLYPHONY
#endif

#ifdef ENABLE_POLYPHONY
	// Reset all voices
	for (int v = 0; v < MAX_POLYPHONY; v++)
		this->Voices[v]->Reset();
#else
	this->Envs[0]->Reset();
	this->Envs[1]->Reset();
#endif

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
