#if !defined(_OBJECTS_H_)
#define _OBJECTS_H_

#include <Audio.h>
#include <effect_hammond_vibrato.h>
extern int systemState;

/*
 * This macro defines the list of objects types we're allowed to create and edit
 * Generated from \Arduino\libraries\Audio\gui\index.html by objectExtract.py
 */
#define AUDIO_ENTRIES \
  AUDIO_ENTRY(AudioAnalyzeEvent,Event,AUDIO_ANALYZE_EVENT,1,1,analyze,,) \
  AUDIO_ENTRY(AudioAnalyzeFFT1024,FFT1024,AUDIO_ANALYZE_FFT1024,1,0,analyze,ff1k,) \
  AUDIO_ENTRY(AudioAnalyzeFFT256,FFT256,AUDIO_ANALYZE_FFT256,1,0,analyze,f256,) \
  AUDIO_ENTRY(AudioAnalyzeNoteFrequency,NoteFrequency,AUDIO_ANALYZE_NOTE_FREQUENCY,1,0,analyze,nfrq,) \
  AUDIO_ENTRY(AudioAnalyzePeak,Peak,AUDIO_ANALYZE_PEAK,1,0,analyze,peak,) \
  AUDIO_ENTRY(AudioAnalyzePrint,Print,AUDIO_ANALYZE_PRINT,1,0,analyze,prnt,) \
  AUDIO_ENTRY(AudioAnalyzeRMS,RMS,AUDIO_ANALYZE_RMS,1,0,analyze,rms,) \
  AUDIO_ENTRY(AudioAnalyzeToneDetect,ToneDetect,AUDIO_ANALYZE_TONE_DETECT,1,0,analyze,tone,) \
  AUDIO_ENTRY(AudioEffectBitcrusher,Bitcrusher,AUDIO_EFFECT_BITCRUSHER,1,1,effect,crsh,) \
  AUDIO_ENTRY(AudioEffectChorus,Chorus,AUDIO_EFFECT_CHORUS,1,1,effect,chor,) \
  AUDIO_ENTRY(AudioEffectDelay,Delay,AUDIO_EFFECT_DELAY,1,8,effect,dely,) \
  AUDIO_ENTRY(AudioEffectDelayExternal,DelayExternal,AUDIO_EFFECT_DELAY_EXTERNAL,1,8,effect,delX,AEDE_CONSTRUCTOR) \
  AUDIO_ENTRY(AudioEffectDigitalCombine,DigitalCombine,AUDIO_EFFECT_DIGITAL_COMBINE,2,1,effect,cmbn,) \
  AUDIO_ENTRY(AudioEffectEnvelope,Envelope,AUDIO_EFFECT_ENVELOPE,1,1,effect,envL,) \
  AUDIO_ENTRY(AudioEffectExpEnvelope,ExpEnvelope,AUDIO_EFFECT_EXP_ENVELOPE,1,1,effect,envE,) \
  AUDIO_ENTRY(AudioEffectFade,Fade,AUDIO_EFFECT_FADE,1,1,effect,fade,) \
  AUDIO_ENTRY(AudioEffectFlange,Flange,AUDIO_EFFECT_FLANGE,1,1,effect,flng,) \
  AUDIO_ENTRY(AudioEffectFreeverb,Freeverb,AUDIO_EFFECT_FREEVERB,1,1,effect,frvb,) \
  AUDIO_ENTRY(AudioEffectFreeverbStereo,FreeverbStereo,AUDIO_EFFECT_FREEVERB_STEREO,1,2,effect,frvS,) \
  AUDIO_ENTRY(AudioEffectGranular,Granular,AUDIO_EFFECT_GRANULAR,1,1,effect,gran,) \
  AUDIO_ENTRY(AudioEffectHammondVibrato,HammondVibrato,AUDIO_EFFECT_HAMMOND_VIBRATO,1,1,effect,hvib,) \
  AUDIO_ENTRY(AudioEffectMidSide,MidSide,AUDIO_EFFECT_MID_SIDE,2,2,effect,mdsd,) \
  AUDIO_ENTRY(AudioEffectMultiply,Multiply,AUDIO_EFFECT_MULTIPLY,2,1,effect,mply,) \
  AUDIO_ENTRY(AudioEffectRectifier,Rectifier,AUDIO_EFFECT_RECTIFIER,1,1,effect,rect,) \
  AUDIO_ENTRY(AudioEffectReverb,Reverb,AUDIO_EFFECT_REVERB,1,1,effect,rvrb,) \
  AUDIO_ENTRY(AudioEffectWaveFolder,WaveFolder,AUDIO_EFFECT_WAVE_FOLDER,2,1,effect,fold,) \
  AUDIO_ENTRY(AudioEffectWaveshaper,Waveshaper,AUDIO_EFFECT_WAVESHAPER,1,1,effect,shpr,) \
  AUDIO_ENTRY(AudioFilterBiquad,Biquad,AUDIO_FILTER_BIQUAD,1,1,filter,biqd,) \
  AUDIO_ENTRY(AudioFilterFIR,FIR,AUDIO_FILTER_FIR,1,1,filter,fir,) \
  AUDIO_ENTRY(AudioFilterLadder,Ladder,AUDIO_FILTER_LADDER,3,1,filter,ladr,) \
  AUDIO_ENTRY(AudioFilterStateVariable,StateVariable,AUDIO_FILTER_STATE_VARIABLE,2,3,filter,svf,) \
  AUDIO_ENTRY(AudioAmplifier,Amplifier,AUDIO_AMPLIFIER,1,1,mixer,amp,) \
  AUDIO_ENTRY(AudioMixer,Mixer,AUDIO_MIXER,8,1,mixer,mixN,8) \
  AUDIO_ENTRY(AudioMixer4,Mixer4,AUDIO_MIXER4,4,1,mixer,mix4,) \
  AUDIO_ENTRY(AudioMixerStereo,MixerStereo,AUDIO_MIXER_STEREO,8,2,mixer,mixS,8) \
  AUDIO_ENTRY(AudioSynthKarplusStrong,KarplusStrong,AUDIO_SYNTH_KARPLUS_STRONG,0,1,synth,kpst,) \
  AUDIO_ENTRY(AudioSynthNoisePink,NoisePink,AUDIO_SYNTH_NOISE_PINK,0,1,synth,npnk,) \
  AUDIO_ENTRY(AudioSynthNoiseWhite,NoiseWhite,AUDIO_SYNTH_NOISE_WHITE,0,1,synth,nwht,) \
  AUDIO_ENTRY(AudioSynthSimpleDrum,SimpleDrum,AUDIO_SYNTH_SIMPLE_DRUM,0,1,synth,drum,) \
  AUDIO_ENTRY(AudioSynthToneSweep,ToneSweep,AUDIO_SYNTH_TONE_SWEEP,0,1,synth,tswp,) \
  AUDIO_ENTRY(AudioSynthWaveform,Waveform,AUDIO_SYNTH_WAVEFORM,0,1,synth,wavf,) \
  AUDIO_ENTRY(AudioSynthWaveformDc,WaveformDc,AUDIO_SYNTH_WAVEFORM_DC,0,1,synth,dc,) \
  AUDIO_ENTRY(AudioSynthWaveformModulated,WaveformModulated,AUDIO_SYNTH_WAVEFORM_MODULATED,2,1,synth,wvmd,) \
  AUDIO_ENTRY(AudioSynthWaveformPWM,WaveformPWM,AUDIO_SYNTH_WAVEFORM_PWM,1,1,synth,wpwm,) \
  AUDIO_ENTRY(AudioSynthWaveformSine,WaveformSine,AUDIO_SYNTH_WAVEFORM_SINE,0,1,synth,wsin,) \
  AUDIO_ENTRY(AudioSynthWaveformSineHires,WaveformSineHires,AUDIO_SYNTH_WAVEFORM_SINE_HIRES,0,2,synth,wshr,) \
  AUDIO_ENTRY(AudioSynthWaveformSineModulated,WaveformSineModulated,AUDIO_SYNTH_WAVEFORM_SINE_MODULATED,1,1,synth,wsmd,) \
  AUDIO_ENTRY(AudioSynthWavetable,Wavetable,AUDIO_SYNTH_WAVETABLE,0,1,synth,wtab,) \


/*
  AUDIO_ENTRY(AudioControlAK4558,ControlAK4558,AUDIO_CONTROL_AK4558,0,0,control,ctrl,) \
  AUDIO_ENTRY(AudioControlCS42448,ControlCS42448,AUDIO_CONTROL_CS42448,0,0,control,ctrl,) \
  AUDIO_ENTRY(AudioControlCS4272,ControlCS4272,AUDIO_CONTROL_CS4272,0,0,control,ctrl,) \
  AUDIO_ENTRY(AudioControlSGTL5000,ControlSGTL5000,AUDIO_CONTROL_SGTL5000,0,0,control,ctrl,) \
  AUDIO_ENTRY(AudioControlWM8731,ControlWM8731,AUDIO_CONTROL_WM8731,0,0,control,ctrl,) \
  AUDIO_ENTRY(AudioControlWM8731master,ControlWM8731master,AUDIO_CONTROL_WM8731MASTER,0,0,control,ctrl,) \
  AUDIO_ENTRY(AsyncAudioInputSPDIF3,AsyncInputSPDIF3,ASYNC_AUDIO_INPUT_SPDIF3,0,2,input,SPDIF,) \
  AUDIO_ENTRY(AudioInputAnalog,InputAnalog,AUDIO_INPUT_ANALOG,0,1,input,ANAi,) \
  AUDIO_ENTRY(AudioInputAnalogStereo,InputAnalogStereo,AUDIO_INPUT_ANALOG_STEREO,0,2,input,ANAi,) \
  AUDIO_ENTRY(AudioInputI2S,InputI2S,AUDIO_INPUT_I2S,0,2,input,I2Si,) \
  AUDIO_ENTRY(AudioInputI2S2,InputI2S2,AUDIO_INPUT_I2S2,0,2,input,I2Si,) \
  AUDIO_ENTRY(AudioInputI2SHex,InputI2SHex,AUDIO_INPUT_I2SHEX,0,6,input,I2Si,) \
  AUDIO_ENTRY(AudioInputI2SOct,InputI2SOct,AUDIO_INPUT_I2SOCT,0,8,input,I2Si,) \
  AUDIO_ENTRY(AudioInputI2SQuad,InputI2SQuad,AUDIO_INPUT_I2SQUAD,0,4,input,I2Si,) \
  AUDIO_ENTRY(AudioInputI2Sslave,InputI2Sslave,AUDIO_INPUT_I2SSLAVE,0,2,input,I2Si,) \
  AUDIO_ENTRY(AudioInputPDM,InputPDM,AUDIO_INPUT_PDM,0,1,input,PDMi,) \
  AUDIO_ENTRY(AudioInputPDM2,InputPDM2,AUDIO_INPUT_PDM2,0,1,input,PDM2i,) \
  AUDIO_ENTRY(AudioInputSPDIF3,InputSPDIF3,AUDIO_INPUT_SPDIF3,0,2,input,SPDIF,) \
  AUDIO_ENTRY(AudioInputTDM,InputTDM,AUDIO_INPUT_TDM,0,16,input,TDMi,) \
  AUDIO_ENTRY(AudioInputTDM2,InputTDM2,AUDIO_INPUT_TDM2,0,16,input,TDM2i,) \
  AUDIO_ENTRY(AudioInputUSB,InputUSB,AUDIO_INPUT_USB,0,2,input,USBi,) \
  AUDIO_ENTRY(AudioOutputADAT,OutputADAT,AUDIO_OUTPUT_ADAT,8,0,output,ADATo,) \
  AUDIO_ENTRY(AudioOutputAnalog,OutputAnalog,AUDIO_OUTPUT_ANALOG,1,0,output,ANAo,) \
  AUDIO_ENTRY(AudioOutputAnalogStereo,OutputAnalogStereo,AUDIO_OUTPUT_ANALOG_STEREO,2,0,output,ANAo,) \
  AUDIO_ENTRY(AudioOutputI2S,OutputI2S,AUDIO_OUTPUT_I2S,2,0,output,I2So,) \
  AUDIO_ENTRY(AudioOutputI2S2,OutputI2S2,AUDIO_OUTPUT_I2S2,2,0,output,I2So,) \
  AUDIO_ENTRY(AudioOutputI2SHex,OutputI2SHex,AUDIO_OUTPUT_I2SHEX,6,0,output,I2So,) \
  AUDIO_ENTRY(AudioOutputI2SOct,OutputI2SOct,AUDIO_OUTPUT_I2SOCT,8,0,output,I2So,) \
  AUDIO_ENTRY(AudioOutputI2SQuad,OutputI2SQuad,AUDIO_OUTPUT_I2SQUAD,4,0,output,I2So,) \
  AUDIO_ENTRY(AudioOutputI2Sslave,OutputI2Sslave,AUDIO_OUTPUT_I2SSLAVE,2,0,output,I2So,) \
  AUDIO_ENTRY(AudioOutputMQS,OutputMQS,AUDIO_OUTPUT_MQS,2,0,output,MQSo,) \
  AUDIO_ENTRY(AudioOutputPT8211,OutputPT8211,AUDIO_OUTPUT_PT8211,2,0,output,PT8211,) \
  AUDIO_ENTRY(AudioOutputPT8211_2,OutputPT8211_2,AUDIO_OUTPUT_PT8211_2,2,0,output,PT8211,) \
  AUDIO_ENTRY(AudioOutputPWM,OutputPWM,AUDIO_OUTPUT_PWM,1,0,output,PWMo,) \
  AUDIO_ENTRY(AudioOutputSPDIF,OutputSPDIF,AUDIO_OUTPUT_SPDIF,2,0,output,SPDIF,) \
  AUDIO_ENTRY(AudioOutputSPDIF2,OutputSPDIF2,AUDIO_OUTPUT_SPDIF2,2,0,output,SPDIF,) \
  AUDIO_ENTRY(AudioOutputSPDIF3,OutputSPDIF3,AUDIO_OUTPUT_SPDIF3,2,0,output,SPDIF,) \
  AUDIO_ENTRY(AudioOutputTDM,OutputTDM,AUDIO_OUTPUT_TDM,16,0,output,TDMo,) \
  AUDIO_ENTRY(AudioOutputTDM2,OutputTDM2,AUDIO_OUTPUT_TDM2,16,0,output,TDM2o,) \
  AUDIO_ENTRY(AudioOutputUSB,OutputUSB,AUDIO_OUTPUT_USB,2,0,output,USBo,) \
*/

#define AEDE_CONSTRUCTOR AUDIO_MEMORY_EXTMEM,1000.0f

enum AudioCategory_e { AudioCategory_none, AudioCategory_patchcord, AudioCategory_analyze, AudioCategory_effect, AudioCategory_filter, AudioCategory_mixer, AudioCategory_synth, AudioCategory_control, AudioCategory_input, AudioCategory_output };

// Cherry-pick from the above list for the I/O we are going to use:
#define MY_AUDIO_IO \
  AUDIO_ENTRY(AudioControlSGTL5000,ControlSGTL5000,AUDIO_CONTROL_SGTL5000,0,0,control,ctrl,) \
  AUDIO_ENTRY(AudioInputI2S,InputI2S,AUDIO_INPUT_I2S,0,2,input,I2Si,) \
  AUDIO_ENTRY(AudioOutputI2S,OutputI2S,AUDIO_OUTPUT_I2S,2,0,output,I2So,) \


// macro to force expansion of parameter list, if needed
#define xAUDIO_ENTRY(x) AUDIO_ENTRY(x)

// Define enum with a unique ID for each object type,
// plus guard values at the ends: IDs are thus 1..N
#define AUDIO_ENTRY(a,b,c,...) c##_ID,
enum AudioID {AUDIO_NONE_ID, AUDIO_ENTRIES AUDIO_MAX_ID, MY_AUDIO_IO AUDIO_LOAD_MAX};
#undef AUDIO_ENTRY

// Count of possible object types, excluding the I/O objects
#define COUNT_OF_objList ((int) (AUDIO_MAX_ID - 1))

// Count of EVERY possible object type, including the I/O objects
#define COUNT_OF_objTypes ((int) (AUDIO_LOAD_MAX - 1))

// Union which can hold a pointer to any object type, referred
// to by its shortened name
#define AUDIO_ENTRY(a,b,c,d,e,...) a* b;
union AudioObjPtr_u {AudioStream* streamObj; AUDIO_ENTRIES MY_AUDIO_IO};
#undef AUDIO_ENTRY

struct AudioObj_t
{
  AudioObjPtr_u o; //!< pointer to the object itself
  AudioID id;     //!< the object's ID number
};

enum class AudioEditMode {destructor = -1, constructor, 
                          enter,edit,exit, 
                          MIDIenter,MIDIedit,MIDIexit, 
                          getParams,setParams, 
                          getMIDIparams,setMIDIparams,processMIDIevent,checkIfActive};
struct getSetParams {char* buffer; size_t sz;};
class AudioObjInstance; // forward declaration needed for editFn()
struct AudioObjStatic_t
{
  const char* name;
  const char* label;
  const AudioCategory_e category;
  const int8_t inputs,outputs;
  const AudioID id;
  int (* const editFn)(AudioObjInstance* aoi, AudioEditMode mode, void* params);
};


#define INIT_OBJ_STATIC_DATA(a,b,c,d,e,f,g) {.name = #b, .label = #g, .category = AudioCategory_##f, \
                                             .inputs = d, .outputs = e, .id = c##_ID, .editFn = edit##b},


class AudioObjInstance
{
  public:
    AudioObjInstance(AudioObjStatic_t& o, int16_t _x, int16_t _y, bool _noD, bool _isAcopy = false);
    AudioObjInstance(AudioObjStatic_t& o, int16_t _x, int16_t _y) : AudioObjInstance(o,_x,_y,false,false) {}
    AudioObjInstance(AudioObjStatic_t& o) : AudioObjInstance(o,-9999,-9999,false,true) {}
    ~AudioObjInstance();
    AudioObjStatic_t* objP;
    AudioObjPtr_u streamP;
    void* context; // point to "stuff" needed by this instance: different for each audio object class
    int16_t x;
    int16_t y;
    uint32_t inputAvailFlags; // let's be optimistic!
    bool noDelete;
    bool perVoice; // create copy of this object when new voice is triggered
    bool isAcopy; // true if this object is a copy of one from the main design
    void copySettingsTo(AudioObjInstance& aoi);
    bool isCopyOf(AudioObjInstance& aoi);
};

struct AudioObjInstancePtr {AudioObjInstance* p; };


class PatchcordInstance_t
{
  public:
    PatchcordInstance_t(AudioObjInstance* s, int8_t sp, AudioObjInstance* d, int8_t dp) 
      : conn(new AudioConnection), src(s), dst(d), src_port(sp), dst_port(dp)
      {
        connect();
      }
    PatchcordInstance_t() : PatchcordInstance_t(nullptr, -1, nullptr, -1) {}
    ~PatchcordInstance_t();
    AudioConnection* conn; // the actual connection
    // duplicate of inaccessible information
    AudioObjInstance* src;
    AudioObjInstance* dst;
    int8_t src_port;
    int8_t dst_port;
    void connect(void); 
};

bool operator<(const AudioObjInstancePtr& lhs, const AudioObjInstancePtr& rhs);
extern int objNameToID(const char* s);
extern void editTogglePerVoice(AudioObjInstance* aoi);
extern int editSetStreamParams(AudioObjInstance& aoi);
extern void CopyContext(void* src, void* dst);

#endif // !defined(_OBJECTS_H_)
