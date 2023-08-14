#if !defined(_OBJECTS_H_)
#define _OBJECTS_H_

#include <Audio.h>

#define AUDIO_ENTRIES \
  AUDIO_ENTRY(AudioEffectBitcrusher,Bitcrusher,AUDIO_EFFECT_BITCRUSHER,1,1,effect,crsh) \
  AUDIO_ENTRY(AudioEffectChorus,Chorus,AUDIO_EFFECT_CHORUS,1,1,effect,chor) \
  AUDIO_ENTRY(AudioEffectDelay,Delay,AUDIO_EFFECT_DELAY,1,8,effect,dely) \
  AUDIO_ENTRY(AudioEffectDelayExternal,DelayExternal,AUDIO_EFFECT_DELAY_EXTERNAL,1,8,effect,delX) \
  AUDIO_ENTRY(AudioEffectDigitalCombine,DigitalCombine,AUDIO_EFFECT_DIGITAL_COMBINE,2,1,effect,cmbn) \
  AUDIO_ENTRY(AudioEffectEnvelope,Envelope,AUDIO_EFFECT_ENVELOPE,1,1,effect,envL) \
  AUDIO_ENTRY(AudioEffectExpEnvelope,ExpEnvelope,AUDIO_EFFECT_EXP_ENVELOPE,1,1,effect,envE) \
  AUDIO_ENTRY(AudioEffectFade,Fade,AUDIO_EFFECT_FADE,1,1,effect,fade) \
  AUDIO_ENTRY(AudioEffectFlange,Flange,AUDIO_EFFECT_FLANGE,1,1,effect,flng) \
  AUDIO_ENTRY(AudioEffectFreeverb,Freeverb,AUDIO_EFFECT_FREEVERB,1,1,effect,frvb) \
  AUDIO_ENTRY(AudioEffectFreeverbStereo,FreeverbStereo,AUDIO_EFFECT_FREEVERB_STEREO,1,2,effect,frvS) \
  AUDIO_ENTRY(AudioEffectGranular,Granular,AUDIO_EFFECT_GRANULAR,1,1,effect,gran) \
  AUDIO_ENTRY(AudioEffectMidSide,MidSide,AUDIO_EFFECT_MID_SIDE,2,2,effect,mdsd) \
  AUDIO_ENTRY(AudioEffectMultiply,Multiply,AUDIO_EFFECT_MULTIPLY,2,1,effect,mply) \
  AUDIO_ENTRY(AudioEffectRectifier,Rectifier,AUDIO_EFFECT_RECTIFIER,1,1,effect,rect) \
  AUDIO_ENTRY(AudioEffectReverb,Reverb,AUDIO_EFFECT_REVERB,1,1,effect,rvrb) \
  AUDIO_ENTRY(AudioEffectWaveFolder,WaveFolder,AUDIO_EFFECT_WAVE_FOLDER,2,1,effect,fold) \
  AUDIO_ENTRY(AudioEffectWaveshaper,Waveshaper,AUDIO_EFFECT_WAVESHAPER,1,1,effect,shpr) \
  AUDIO_ENTRY(AudioFilterBiquad,FilterBiquad,AUDIO_FILTER_BIQUAD,1,1,filter,biqd) \
  AUDIO_ENTRY(AudioFilterFIR,FilterFIR,AUDIO_FILTER_F_I_R,1,1,filter,fir) \
  AUDIO_ENTRY(AudioFilterLadder,FilterLadder,AUDIO_FILTER_LADDER,3,1,filter,ladr) \
  AUDIO_ENTRY(AudioFilterStateVariable,FilterStateVariable,AUDIO_FILTER_STATE_VARIABLE,2,3,filter,svf) \
  AUDIO_ENTRY(AudioAmplifier,Amplifier,AUDIO_AMPLIFIER,1,1,mixer,amp) \
  AUDIO_ENTRY(AudioMixer,Mixer,AUDIO_MIXER,1,1,mixer,mixN) \
  AUDIO_ENTRY(AudioMixer4,Mixer4,AUDIO_MIXER4,4,1,mixer,mix4) \
  AUDIO_ENTRY(AudioMixerStereo,MixerStereo,AUDIO_MIXER_STEREO,1,2,mixer,mixS) \
  AUDIO_ENTRY(AudioSynthKarplusStrong,KarplusStrong,AUDIO_SYNTH_KARPLUS_STRONG,0,1,synth,kpst) \
  AUDIO_ENTRY(AudioSynthNoisePink,NoisePink,AUDIO_SYNTH_NOISE_PINK,0,1,synth,npnk) \
  AUDIO_ENTRY(AudioSynthNoiseWhite,NoiseWhite,AUDIO_SYNTH_NOISE_WHITE,0,1,synth,nwht) \
  AUDIO_ENTRY(AudioSynthSimpleDrum,SimpleDrum,AUDIO_SYNTH_SIMPLE_DRUM,0,1,synth,drum) \
  AUDIO_ENTRY(AudioSynthToneSweep,ToneSweep,AUDIO_SYNTH_TONE_SWEEP,0,1,synth,tswp) \
  AUDIO_ENTRY(AudioSynthWaveform,Waveform,AUDIO_SYNTH_WAVEFORM,0,1,synth,wavf) \
  AUDIO_ENTRY(AudioSynthWaveformDc,WaveformDc,AUDIO_SYNTH_WAVEFORM_DC,0,1,synth,dc) \
  AUDIO_ENTRY(AudioSynthWaveformModulated,WaveformModulated,AUDIO_SYNTH_WAVEFORM_MODULATED,2,1,synth,wvmd) \
  AUDIO_ENTRY(AudioSynthWaveformPWM,WaveformPWM,AUDIO_SYNTH_WAVEFORM_PWM,1,1,synth,wpwm) \
  AUDIO_ENTRY(AudioSynthWaveformSine,WaveformSine,AUDIO_SYNTH_WAVEFORM_SINE,0,1,synth,wsin) \
  AUDIO_ENTRY(AudioSynthWaveformSineHires,WaveformSineHires,AUDIO_SYNTH_WAVEFORM_SINE_HIRES,0,2,synth,wshr) \
  AUDIO_ENTRY(AudioSynthWaveformSineModulated,WaveformSineModulated,AUDIO_SYNTH_WAVEFORM_SINE_MODULATED,1,1,synth,wsmd) \
  AUDIO_ENTRY(AudioSynthWavetable,Wavetable,AUDIO_SYNTH_WAVETABLE,0,1,synth,wtab) \



enum AudioCategory_e { AudioCategory_none, AudioCategory_effect, AudioCategory_filter, AudioCategory_mixer, AudioCategory_synth };


#define xAUDIO_ENTRY(x) AUDIO_ENTRY(x)

#define AUDIO_ENTRY(a,b,c,...) c##_ID,
enum AudioIDs {AUDIO_NONE_ID, AUDIO_ENTRIES AUDIO_MAX_ID};
#undef AUDIO_ENTRY


#define AUDIO_ENTRY(a,b,c,d,e,...) a* b;
union AudioObjPtr_u {AUDIO_ENTRIES};
#undef AUDIO_ENTRY

struct AudioObj_t
{
  AudioObjPtr_u o; //!< pointer to the object itself
  AudioIDs id;     //!< the object's ID number
};

struct AudioObjStatic_t
{
  const char* name;
  const char* label;
  const AudioCategory_e category;
  const int8_t inputs,outputs;
};

#define INIT_OBJ_STATIC_DATA(a,b,c,d,e,f,g) {.name = #b, .label = #g, .category = AudioCategory_##f, .inputs = d, .outputs = e},

class AudioObjInstance
{
  public:
    AudioObjInstance(AudioObjStatic_t& o, int16_t _x, int16_t _y) : objP(&o), x(_x),y(_y) {}
    AudioObjStatic_t* objP;
    int16_t x;
    int16_t y;
};

#endif // !defined(_OBJECTS_H_)
