#pragma once
#pragma warning (disable: 4996)

#include "DistrhoPlugin.hpp"

#include "Defines.h"
#include "Structures.h"
#include "GlobalFunctions.h"

#include "SynthOscillator.h"
#include "SynthEnvelope.h"
#include "SynthLfo.h"

#include "FilterMoog.h"
#include "FilterMoog2.h"
#include "FilterDirty.h"
#include "FilterCh12db.h"
#include "Filter303.h"
#include "Filter8580.h"
#include "FilterBiquad.h"
#include "MidiStack.h"

#include "VST2.4_Compatibility.hpp"

class CCetoneSynth : public DISTRHO::Plugin
{
public:

	CCetoneSynth();
	~CCetoneSynth(void);

protected:
	// ----------------------------------------------------------------------------------------------------------------
	// Information

	/**
		Get the plugin label.@n
		This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
	*/
	const char* getLabel() const noexcept override
	{
		return DISTRHO_PLUGIN_NAME;
	}

	/**
		   Get an extensive comment/description about the plugin.@n
		   Optional, returns nothing by default.
		 */
	const char* getDescription() const override
	{
		return "Light-weight multifunctional synthesizer";
	}

	/**
		   Get the plugin author/maker.
		 */
	const char* getMaker() const noexcept override
	{
		return DISTRHO_PLUGIN_BRAND;
	}

	/**
		   Get the plugin license (a single line of text or a URL).@n
		   For commercial plugins this should return some short copyright information.
		 */
	const char* getLicense() const noexcept override
	{
		return "GPLv3";
	}

	/**
		Get the plugin version, in hexadecimal.
		@see d_version()
		*/
	uint32_t getVersion() const noexcept override
	{
		return d_version(1, 0, 0);
	}

	/**
		   Get the plugin unique Id.@n
		   This value is used by LADSPA, DSSI and VST plugin formats.
		   @see d_cconst()
		 */
	int64_t getUniqueId() const noexcept override
	{
		return d_cconst('c', 't', 'n', 'l');
	}

	// ----------------------------------------------------------------------------------------------------------------
	// Init

	void initParameter(uint32_t index, Parameter& parameter) override;

	// ----------------------------------------------------------------------------------------------------------------
	// Internal data

	float getParameterValue(uint32_t index) const override;
	void setParameterValue(uint32_t index, float value) override;

	// ----------------------------------------------------------------------------------------------------------------
	// Audio/MIDI Processing

	void activate() override;
	void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

	// ----------------------------------------------------------------------------------------------------------------
	// Callbacks (optional)

	void bufferSizeChanged(int newBufferSize);
	void sampleRateChanged(double newSampleRate) override;

protected:
	// ----------------------------------------------------------------------------------------------------------------
	// Legacy VST 2.4 interface
	// DPF functions will call them when needed.

	virtual void		resume();

	//virtual VstInt32	processEvents(VstEvents* events);
	virtual VstInt32	processEvents(const DISTRHO::MidiEvent* events, uint32_t eventCount);
	virtual void		process(float **inputs, float **outputs, VstInt32 sampleFrames); 	
	virtual void		processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);

	[[maybe_unused]] virtual VstInt32	getChunk(void** data, bool isPreset = false); 
	[[maybe_unused]] virtual VstInt32	setChunk(void* data, VstInt32 byteSize, bool isPreset = false);

	virtual void		setProgram(VstInt32 program);
	virtual VstInt32	getProgram();
	virtual void		setProgramName(char* name);
	virtual void		getProgramName(char* name);
	virtual bool		getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);

	virtual VstInt32	getNumMidiInputChannels();
	virtual VstInt32	getNumMidiOutputChannels();

	virtual void		setSampleRate (float sampleRate);
	virtual void		setBlockSize (VstInt32 blockSize);

	virtual VstInt32	getVendorVersion ();
	virtual bool		getEffectName(char* name);
	virtual bool		getVendorString(char* text);
	virtual bool		getProductString(char* text);

	virtual VstInt32	canDo(char* text);

	virtual void		setParameter(VstInt32 index, float value);
	virtual float		getParameter(VstInt32 index) const;
	virtual void		getParameterLabel(VstInt32 index, char* label);
	virtual void		getParameterDisplay(VstInt32 index, char* text);
	virtual void		getParameterName(VstInt32 index, char* text);
	void setParameterAutomated(VstInt32 index, float value) { this->setParameterValue(index, value); }

public:
	// Public statics

	static float		SineTable[65536]; 
	static float		SmallSineTable[WAVETABLE_LENGTH];
	static float		FreqTable[PITCH_MAX];
	static int			FreqTableInt[PITCH_MAX];

	static int			LookupTable[65536];
	static float		SawTable[NOTE_MAX * WAVETABLE_LENGTH];
	static float		ParabolaTable[NOTE_MAX * WAVETABLE_LENGTH];
	static int			FreqStepInt[PITCH_MAX];
	static int			FreqStepFrac[PITCH_MAX];

	static float		Pw2Float[4096];
	static int			Pw2Offset[4096];
	static float		Pw2WaveOffset[4096];

	static float		Int2FloatTab[65536];
	static float		Int2FloatTab2[65536];

	static float		SampleRate;
	static float		SampleRate2;
	static float		SampleRate_1;
	static float		SampleRate2_1;
	static float		SampleRatePi;
	static float		Pi;
	static int			C64Arps[8][16];

private:

	// Classes

	CSynthOscillator*	Oscs[3];
	CSynthEnvelope*		Envs[2];
	CSynthLfo*			Lfo;

	CMidiStack*			MidiStack;
	CFilterDirty*		FilterDirty;
	CFilterMoog*		FilterMoog;
	CFilterMoog2*		FilterMoog2;
	CFilterCh12db*		FilterCh12db;
	CFilter303*			Filter303;
	CFilter8580*		Filter8580;
	CFilterBiquad*		FilterBiquad;

	// Variables

	SynthProgram		Programs[129];

	// Program variables

	float				Volume;
	float				Panning;

	int					MainCoarse;
	int					MainFine;

	int					FilterType;
	int					FilterMode;
	float				Cutoff;
	float				Resonance;

	int					ArpMode;
	int					ArpSpeed;

	bool				PortaMode;
	float				PortaSpeed;

	SynthVoice			Voice[3];

	float				EnvAttack[2];
	float				EnvHold[2];
	float				EnvDecay[2];
	float				EnvSustain[2];
	float				EnvRelease[2];

	float				LfoSpeed;
	int					LfoWave;
	int					LfoPw;
	bool				LfoTrigger;
	
	SynthModulation		Modulations[4];

	float				EnvMod;

	// Runtime variables

	float				ModChangeSamples;

	int					CurrentNote;
	int					CurrentVelocity;
	int					CurrentDelta;
	int					CurrentCtrl1;
	int					CurrentProgram;
	
	float				VelocityMod;
	float				Ctrl1Mod;

	float				VelocityModStep;
	float				Ctrl1ModStep;

	float				VelocityModEnd;
	float				Ctrl1ModEnd;

	int					LastP0;
	int					NextP0;
	int					NextP1;
	int					NextP2;

	int					FilterCounter;

	int					ArpPos;
	int					ArpDelay;
	int					ArpCounter;

	int					CurrentPitch;
	int					PortaStep;
	int					PortaPitch;
	int					PortaFrac;
	float				PortaSamples;
	bool				DoPorta;

	float				CutoffDest;
	float				CutoffStep;

	int					VoicePulsewidth[3];
	
	float				LfoPitch;

	// Init functions
	
	void				InitSineTable();
	void				InitFreqTable();
	void				InitSynthParameters();

	// Programs

	void				ReadProgram(int prg);
	void				WriteProgram(int prg);

	// Synthesizer functions	(CetoneSynthMain.cpp)

	void				SynthProcess(float **inputs, float **outputs, VstInt32 sampleFrames, bool replace);
	void				HandleMidi(int p0, int p1, int p2);

	void				NoteOn(int note, int vel);
	void				NoteOff(int note, int vel);

	void				UpdateFilters();
	void				UpdateFilters(float cutoff, float q, float mode);
	void				SetFilterMode(int mode);

	void				SetArpSpeed(int ms);
	void				SetPortaSpeed(float speed);
	void				UpdateEnvelopes();
	void				SetCutoffSave(float value);

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CCetoneSynth)
};
