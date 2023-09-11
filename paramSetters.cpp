#include "objects.h"
#include "display.h"
#include "paramsetters.h"
#include "limitedEncoder.h"

extern M5w_8angle ctrl;
#define M5ANGLE_MIN   20
#define M5ANGLE_MAX 4080

// #define BOX_DEF(width,lines) (320/2 - (width)/2),(240/2 - (27+(lines)*16+16)/2),(width),(27+(lines)*16+16)

//===========================================================================================
SettingsEditor* se;
const float LOG_NOTE_A = 0.781359714f;
const ParamEntry freqLimits{nullptr,-1.0f,1.0f}; // special, for setting hook control

//===========================================================================================
size_t milliseconds2bytes(float ms) { return 2*AUDIO_SAMPLE_RATE_EXACT*ms/1000.0f; }

//===========================================================================================
bool ScaleF(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter)
{
  bool result = false;
  float newVal = map((float) raw, M5ANGLE_MIN, M5ANGLE_MAX, pe.min.f, pe.max.f);
  newVal = newVal*filter + pv.value.f*(1.0f - filter);
  if (fabs(newVal - pv.value.f) > (0.005*( pe.max.f - pe.min.f)))
  {
    pv.value.f = constrain(newVal, pe.min.f, pe.max.f);
    result = true;
  }
  return result;
}


bool ScaleFreq(const ParamEntry& pe, ParamValue& pv, int16_t raw, int16_t pow2, float filter)
{
  bool result = false;
  float newVal = map((float) raw, M5ANGLE_MIN, M5ANGLE_MAX, -1.0f, 1.0f); // get a value
  newVal = newVal + (float) pow2 + LOG_NOTE_A;
  newVal = newVal*filter + pv.value.f*(1.0f - filter);
  if (fabs(newVal - pv.value.f) > (0.0005*( pe.max.f - pe.min.f)))
  {
    pv.value.f = constrain(newVal, pe.min.f, pe.max.f);
    result = true;
  }
  return result;
}


bool ScaleI(const ParamEntry& pe, ParamValue& pv, int16_t raw)
{
  bool result = false;
  int newVal = constrain(map(raw, M5ANGLE_MIN, M5ANGLE_MAX, pe.min.i, pe.max.i), pe.min.i, pe.max.i);
  if (newVal!= pv.value.i)
  {
    pv.value.i = newVal;
    result = true;
  }
  return result;
}

bool Scale(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter = 1.0f)
{
  bool result = false;
  switch(pe.ValType)
  { 
    case 'l':     
    case 'f': result = ScaleF(pe,pv,raw, filter); break;
    
    case 'c':
    case 'i': result = ScaleI(pe,pv,raw); break;
  }
  return result;
}

void HookControl(M5w_8angle& ctrl, int ch, const ParamEntry& pe, ParamValue& pv)
{
  switch (pe.ValType)
  {
    case 'l':
    case 'f': ctrl.setHook(ch,map(pv.value.f,pe.min.f,pe.max.f,M5ANGLE_MIN,M5ANGLE_MAX)); break;
    
    case 'c':
    case 'i':  ctrl.setHook(ch,map(pv.value.i,pe.min.i,pe.max.i,M5ANGLE_MIN,M5ANGLE_MAX)); break;
  }
}

int testExit(uint32_t& exitAt)
{
  int result = 1; // assume we'll keep claiming UI
  
  if (encM.getButton())
    exitAt = 1;
  else
  {
    if (exitAt)
    {
      exitAt = 0;
      result = 0;
    }
  }
  return result;
}

//===========================================================================================
void updateFromControls(AudioObjInstance* aoi)
{
  if (nullptr != se)
  {
    for (size_t i=0; i < se->paramCount; i++)
    {
      if (Scale(se->params[i],se->aray[i],ctrl.getPot16(i),0.999f))
      {
        se->ShowValue(i);
      }
    }
  }
}

//===========================================================================================
void SettingsEditor::Init(const char* title)
{
  display.ShowTitle(title,5,5);
  ShowPage();
}


void SettingsEditor::ShowPage(void)
{
  int row = 0;
  size_t first = 0, nCtrl = paramCount;

  if (nullptr != pages)
  {
    first = pages[currentPage].start;
    nCtrl = pages[currentPage].count;
  }
  
  for (size_t i = 0; i < nCtrl; i++)
  {
    if (0 != params[i+first].xoff) /// if we have an X offset
      row--; // we're on the same row as before
    ShowLabel(i+first,row,5,27);
    ShowValue(i+first);
    if (!ctrl.isHooking(i)) // specialized enterEditMode() may have hooked already - don't re-do
      HookControl(ctrl,i,params[i+first],aray[i+first]);
    row++;            
  }
}

// Change currentPage to a new parameters page number, if possible
// Does not update screen
// \return true if valid page number
bool SettingsEditor::ChangePage(int newPage)
{
  bool result = false;
  
  if (nullptr != pages && newPage >= 0)
  {
    int i = 0;
    size_t firstParam = 0;

    while (i < newPage && firstParam < paramCount)
    {
      firstParam += pages[i].count;
      i++;
    }

    if (i == newPage && firstParam < paramCount)
    {
      currentPage = newPage;
      result = true;
    }
  }
  
  return result;
}


void SettingsEditor::ShowVoiceFlag(bool flag)
{
  display.ShowVoiceFlag(flag);
}

//===========================================================================================
int editGetParamsAny(const ParamEntry* params, const ParamValue* aray, const size_t paramCount, getSetParams* p)
{
  size_t left = p->sz;
  char* ptr = p->buffer;
  int off = 0;
  
  for (size_t i=0; i < paramCount && off >= 0 && left >= 15; i++)
  {
    switch (params[i].ValType)
    {
      case 'l':
      case 'f': off = sprintf(ptr,"%f,",aray[i].value.f); break;
      case 'c':
      case 'i': off = sprintf(ptr,"%d,",aray[i].value.i); break;
    }

    ptr += off;
    left -= off;            
  }
  p->sz = ptr - p->buffer; // amount of buffer used

  return paramCount != 0;
}

int editSetParamsAny(const ParamEntry* params, ParamValue* aray, const size_t paramCount, getSetParams* p)
{
  char* ptr = p->buffer;
  int off = 0;
  
  for (size_t i=0; i < paramCount && off >= 0; i++)
  {
    ValUnion value;
    
    switch (params[i].ValType)
    {
      case 'f':
      case 'l':
        sscanf(ptr,"%f,%n",&value.f,&off);
        if (value.f < params[i].min.f || value.f > params[i].max.f)
          value.f = (params[i].min.f + params[i].max.f) / 2.0f;
        Serial.printf("%s = %.3f ... ",params[i].label,value.f);
        aray[i].value.f = value.f;
        break;
        
      case 'i':
      case 'c':
        sscanf(ptr,"%d,%n",&value.i,&off);
        if (value.i < params[i].min.i || value.i > params[i].max.i)
          value.i = (params[i].min.i + params[i].max.i) / 2;
        Serial.printf("%s = %d ... ",params[i].label,value.i);
        aray[i].value.i = value.i;
        break;
    }

    ptr += off;
  }
  Serial.println();
  p->sz = ptr - p->buffer;
  return 1;
}

//=====================================================================================
// Set AudioStream object's parameters from the stored ones
int editSetStreamParams(AudioObjInstance& aoi)
{
  ContextBase* myContext = (ContextBase*) aoi.context;
  
  // set the actual stream object's parameters
  for (size_t i=0; i < myContext->paramCount; i++)
    myContext->setParam(i,&aoi);

  return (int) myContext->paramCount;
}


//===========================================================================================
// Strong definitions of setup controls
//===========================================================================================
void ContextChorus::allocMem(memRecord& mem, size_t newsize, AudioObjInstance* aoi)
{
  if (nullptr != mem.ptr)
    free(mem.ptr);
  mem.ptr = (short*) malloc(newsize);
  mem.sz  = newsize;
  memset(mem.ptr,0,mem.sz);  
  aoi->streamP.Chorus->begin(mem.ptr,milliseconds2bytes(s.length.value.f)/2,s.voices.value.i);  
  AudioInterrupts();  
}

void ContextChorus::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: 
      {
        size_t newsize = milliseconds2bytes(s.length.value.f);
        if (newsize > mem.sz)
        {
          AudioNoInterrupts();
          allocMem(mem,newsize,aoi);
        }
        else          
          aoi->streamP.Chorus->begin(mem.ptr,milliseconds2bytes(s.length.value.f)/2,s.voices.value.i); 
      }
      break;
    case 1: aoi->streamP.Chorus->voices(s.voices.value.i); break;
  }
}

const ParamEntry ContextChorus::_params[2] = 
{
  {"length [ms]", 10.0f, 200.0f},
  {"     voices", 1, 20},
};

void ContextChorus::enterEditMode(AudioObjInstance* aoi)
{
  tmp = mem;
  AudioNoInterrupts();
  mem.ptr = nullptr; // prevent freeing old chorus memory for now
  allocMem(mem,milliseconds2bytes(params[0].max.f),aoi);
}

template <>
void enterEditMode<ContextChorus>(ContextChorus* myContext, AudioObjInstance* aoi)
{
  myContext->enterEditMode(aoi);
}

void ContextChorus::exitEditMode(AudioObjInstance* aoi)
{
  if (milliseconds2bytes(s.length.value.f) <= tmp.sz) // old allocation is OK for new setting
  {
    aoi->streamP.Chorus->begin(tmp.ptr,milliseconds2bytes(s.length.value.f)/2,s.voices.value.i);
    free(mem.ptr);
    mem = tmp; // back to using old memory allocation
  }
  else // we're gonna need a bigger buffer!
  {
    AudioNoInterrupts();
    allocMem(mem,milliseconds2bytes(s.length.value.f),aoi);
    free(tmp.ptr);
  }  
}

template <>
void exitEditMode<ContextChorus>(ContextChorus* myContext, AudioObjInstance* aoi)
{
  myContext->exitEditMode(aoi);
}

int editChorus(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectChorus, ContextChorus>(aoi,mode,params); 
}

//===========================================================================================
void ContextMixer4::setParam(int i, AudioObjInstance* aoi)
{
  aoi->streamP.Mixer4->gain(i,gains[i].value.f); 
}

const ParamEntry ContextMixer4::_params[4] = 
{
  {"ch1", 0.0f, 1.0f},
  {"ch2", 0.0f, 1.0f},
  {"ch3", 0.0f, 1.0f},
  {"ch4", 0.0f, 1.0f},
};


int editMixer4(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioMixer4, ContextMixer4>(aoi,mode,params);    
}


//===========================================================================================
void ContextMixerStereo::setParam(int i, AudioObjInstance* aoi)
{
  int ch = i / 2;
  if (0 == (i & 1))
    aoi->streamP.MixerStereo->gain(ch,gainOrPan[i].value.f); 
  else
    aoi->streamP.MixerStereo->pan(ch,gainOrPan[i].value.f); 
  
}

const ParamPage ContextMixerStereo::_pages[2] {{0,4},{4,4}};

const ParamEntry ContextMixerStereo::_params[8] = 
{
  {"ch1", 0.0f, 1.0f}, {"pan1", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch2", 0.0f, 1.0f}, {"pan2", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch3", 0.0f, 1.0f}, {"pan3", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch4", 0.0f, 1.0f}, {"pan4", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
};

int editMixerStereo(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioMixerStereo, ContextMixerStereo>(aoi,mode,params);    
}


//===========================================================================================
const ParamChoice velocityShapes[] 
  {{"linear", 0},
   {"curved", 1},
   {"max" ,   2}
  };

const ParamChoice tuningTypes[] 
  {{"equal", 0},
   {"Hammond", 1},
  };

const ParamEntry ContextWaveformModulated::MIDIparams[] 
{
  {"  octave", 0, 9}, // middle C = 261.63Hz = note#60 = octave 4
  {"  detune", -6.00f, +6.00f}, // semitones / cents
  {"velocity",PARAM_ENTRY_CHOICES(velocityShapes)},
  {"  tuning",PARAM_ENTRY_CHOICES(tuningTypes)},
};

const ParamChoice waveShapes[12] = 
  {{"sine",0},
   {"saw" , 1},
   {"square" , 2},
   {"triangle" , 3},
   //{"arbitrary" , 4}, // needs to be loaded, leave for now
   {"pulse" , 5},
   {"saw_rev" , 6},
   {"s&h" , 7},
   {"tri_var" , 8},
   {"saw_bl" , 9},
   {"saw_rev_bl" , 10},
   {"square_bl" , 11},
   {"pulse_bl" , 12}
  };
  
ParamChoice modTypes[] = {{"frequency",0},{"phase",1}};


const ParamEntry ContextWaveformModulated::_params[6] = 
{
  {" waveform", PARAM_ENTRY_CHOICES(waveShapes)},
  {"frequency", -4.0f, 14.0f, 'l'}, // log2(freq) is what we actually store
  {"amplitude", 0.0f, 1.0f},
  {"   offset", -1.0f, 1.0f},
  {" mod type",PARAM_ENTRY_CHOICES(modTypes)},
  {"mod depth",0.0f, 12.0f} // frequency is 0.0 - 12.0 octaves; phase modulation could be 9000°
};

void ContextWaveformModulated::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.WaveformModulated->begin(waveShapes[s.waveform.value.i].value); break;
    case 1: aoi->streamP.WaveformModulated->frequency(pow(2,s.frequency.value.f)); break;
    case 2: aoi->streamP.WaveformModulated->amplitude(s.amplitude.value.f); break;
    case 3: aoi->streamP.WaveformModulated->offset(s.offset.value.f); break;
    /*
    case 2: aoi->streamP.WaveformModulated->amplitude(s.amplitude.value.f); break;
    case 3: aoi->streamP.WaveformModulated->offset(s.offset.value.f); break;
    */
  }
}

template <>
void enterEditMode<ContextWaveformModulated>(ContextWaveformModulated* myContext, AudioObjInstance* aoi)
{
  // fix up the pot and encoder values
  int iprt = floor(myContext->s.frequency.value.f - LOG_NOTE_A);
  float frac = myContext->s.frequency.value.f - iprt - LOG_NOTE_A;

  Serial.printf("freq is %f -> %f Hz\n",myContext->s.frequency.value.f,pow(2,myContext->s.frequency.value.f));
  
  enc0.setLimits(-3,12);
  if (frac > 0.5f)
  {
    frac -= 1.0f;
    iprt++;
  }
  enc0.setValue(iprt);

  ParamValue pv{frac};    
  HookControl(ctrl,1,freqLimits,pv); // frequency pot is #1: set hook

  Serial.printf("Hook set to %f; encoder to %d\n",pv.value.f,iprt);
}
  

template <> // template specialization for setting WaveformModulated; needed for frequency setting
void updateFromControls<ContextWaveformModulated>(ContextWaveformModulated* myContext, AudioObjInstance* aoi)
{
  for (size_t i=0; i < myContext->paramCount; i++)
  {
    if (1 == i) // frequency
    {
      enc0.available();
      if (ScaleFreq(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
      {
        se->ShowValue(i);
        myContext->setParam(i,aoi);
      }      
    }
    else
    {
      if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
      {
        se->ShowValue(i);
        myContext->setParam(i,aoi);
      }
    }
  }
}
  
int editWaveformModulated(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthWaveformModulated, ContextWaveformModulated>(aoi,mode,params);  
}

//===========================================================================================
const ParamEntry ContextWaveformDc::_params[] = {
  {"value", -1.0f, 1.0f},
};


void ContextWaveformDc::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.WaveformDc->amplitude(amplitude.value.f); break;
  }
}

int editWaveformDc(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthWaveformDc, ContextWaveformDc>(aoi,mode,params);    
}

//===========================================================================================
const ParamEntry ContextNoise::_params[] = {
  {"amplitude", 0.0f, 1.0f}
};

void ContextNoise::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: 
      switch (aoi->objP->id)
      {
        case AUDIO_SYNTH_NOISE_WHITE_ID: aoi->streamP.NoiseWhite->amplitude(amplitude.value.f); break;
        case AUDIO_SYNTH_NOISE_PINK_ID:  aoi->streamP.NoisePink ->amplitude(amplitude.value.f); break;
        default: break;
      }
      break;
  }
}

int editNoiseWhite(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthNoiseWhite, ContextNoise>(aoi,mode,params);    
}

int editNoisePink(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthNoisePink, ContextNoise>(aoi,mode,params);    
}

//===========================================================================================
 const ParamChoice inputsSGTL5000[2]  
  {{"line", AUDIO_INPUT_LINEIN},
   {"mic" , AUDIO_INPUT_MIC},
  };
  
 const ParamChoice avcSGTL5000[] = 
  {{"off", 0},
   {"on" , 1},
  };
  
 const ParamChoice inputLevelsSGTL5000[] = 
  {  // Vpk-pk
    {"0.24", 15},
    {"0.29", 14},
    {"0.34", 13},
    {"0.40", 12},
    {"0.48", 11},
    {"0.56", 10},
    {"0.67", 9},
    {"0.79", 8},
    {"0.94", 7},
    {"1.11", 6},
    {"1.33", 5}, // default: index [10]
    {"1.58", 4},
    {"1.87", 3},
    {"2.22", 2},
    {"2.63", 1},
    {"3.12", 0},
  };

 const ParamChoice outputLevelsSGTL5000[] = 
  {  // Vpk-pk
    {"1.16", 31},
    {"1.22", 30},
    {"1.29", 29},
    {"1.37", 28},
    {"1.44", 27},
    {"1.53", 26},
    {"1.62", 25},
    {"1.71", 24},
    {"1.80", 23},
    {"1.91", 22},
    {"2.02", 21},
    {"2.14", 20}, // default: index [11]
    {"2.26", 19},
    {"2.39", 18},
    {"2.53", 17},
    {"2.67", 16},
    {"2.83", 15},
    {"2.98", 14},
    {"3.16", 13}, 
  };
  
     
 const ParamEntry ContextControlSGTL5000::_params[] = 
{
  {"        volume", 0.0f, 1.0f},
  {"  input select", PARAM_ENTRY_CHOICES(inputsSGTL5000)},
  {"  micGain [dB]", 0, 63},
  {"   auto volume", PARAM_ENTRY_CHOICES(avcSGTL5000)},
  {" input [Vpkpk]", PARAM_ENTRY_CHOICES(inputLevelsSGTL5000)},
  {"output [Vpkpk]", PARAM_ENTRY_CHOICES(outputLevelsSGTL5000)},
};

void ContextControlSGTL5000::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  { // volume,inputSelect,micGain,lineInLevel,lineOutLevel,autoVolume
    case 0: aoi->streamP.ControlSGTL5000->volume(s.volume.value.f); break;
    case 1: aoi->streamP.ControlSGTL5000->inputSelect((int) inputsSGTL5000[s.inputSelect.value.i].value); break;
    case 2: aoi->streamP.ControlSGTL5000->micGain(s.micGain.value.i); break;
    
    case 4: aoi->streamP.ControlSGTL5000->lineInLevel(inputLevelsSGTL5000[s.lineInLevel.value.i].value); break;
    case 5: aoi->streamP.ControlSGTL5000->lineOutLevel(outputLevelsSGTL5000[s.lineOutLevel.value.i].value); break;
    
    case 3: 
      if (s.autoVolume.value.i)
      {
        aoi->streamP.ControlSGTL5000->audioPreProcessorEnable();
        aoi->streamP.ControlSGTL5000->autoVolumeEnable();                  
      }
      else
      {
        aoi->streamP.ControlSGTL5000->audioProcessorDisable();
        aoi->streamP.ControlSGTL5000->autoVolumeDisable();                  
      }
      break;
  }  
}

int editControlSGTL5000(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioControlSGTL5000, ContextControlSGTL5000>(aoi,mode,params);
}

//===========================================================================================
 const ParamChoice ladderInterpolation[] = 
  {  
    {"FIR poly", LADDER_FILTER_INTERPOLATION_FIR_POLY},
    {"linear", LADDER_FILTER_INTERPOLATION_LINEAR}
  };
  
void ContextLadder::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Ladder->frequency(pow(2,s.frequency.value.f)); break;
    case 1: aoi->streamP.Ladder->resonance(s.resonance.value.f); break;
    case 2: aoi->streamP.Ladder->octaveControl(s.octaves.value.f); break;
    case 3: aoi->streamP.Ladder->inputDrive(s.drive.value.f); break;
    case 4: aoi->streamP.Ladder->passbandGain(s.gain.value.f); break;
    case 5: aoi->streamP.Ladder->interpolationMethod((AudioFilterLadderInterpolation) ladderInterpolation[s.interpolation.value.i].value); break;
  }
}

const ParamEntry ContextLadder::_params[6]  {
        {"    frequency", 3.0f, 13.2877123795495f, 'l'}, // 8.0 .. 10,000.0 Hz
        {"    resonance", 0.0f, 1.8f},
        {"      octaves", 0.0f, 7.0f},
        {"        drive", 0.0f, 4.0f},
        {"passband gain", 0.0f, 0.5f},
        {"interpolation", PARAM_ENTRY_CHOICES(ladderInterpolation)}
    };

int editLadder(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioFilterLadder, ContextLadder>(aoi,mode,params);
}

//===========================================================================================
void ContextStateVariable::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.StateVariable->frequency(pow(2,s.frequency.value.f)); break;
    case 1: aoi->streamP.StateVariable->resonance(s.resonance.value.f); break;
    case 2: aoi->streamP.StateVariable->octaveControl(s.octaves.value.f); break;
  }
}

const ParamEntry ContextStateVariable::_params[3] = {
        {"frequency", 3.0f, 13.2877123795495f, 'l'}, // 8.0 .. 10,000.0 Hz
        {"resonance", 0.7f, 5.0f},
        {"  octaves", 0.0f, 7.0f}
    };

int editStateVariable(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioFilterStateVariable, ContextStateVariable>(aoi,mode,params);
}

//===========================================================================================
const ParamEntry ContextWaveform::MIDIparams[] 
{
  {"  octave", 0, 9}, // middle C = 261.63Hz = note#60 = octave 4
  {"  detune", -6.00f, +6.00f}, // semitones / cents
  {"velocity",PARAM_ENTRY_CHOICES(velocityShapes)},
  {"  tuning",PARAM_ENTRY_CHOICES(tuningTypes)},
};

const ParamEntry ContextWaveform::_params[] = {
  {"  waveform", PARAM_ENTRY_CHOICES(waveShapes)},
  {" frequency", -4.0f, 14.0f, 'l'}, // log2(freq) is what we actually store
  {" amplitude", 0.0f, 1.0f},
  {"pulseWidth", 0.0f, 1.0f},
  {"    offset", -1.0f, 1.0f},
};


void ContextWaveform::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Waveform->begin(waveShapes[s.waveform.value.i].value); break;
    case 1: aoi->streamP.Waveform->frequency(pow(2,s.frequency.value.f)); break;
    case 2: aoi->streamP.Waveform->amplitude(s.amplitude.value.f); break;
    case 3: aoi->streamP.Waveform->pulseWidth(s.pulseWidth.value.f); break;
    case 4: aoi->streamP.Waveform->offset(s.offset.value.f); break;
  }
}

template <>
void enterEditMode<ContextWaveform>(ContextWaveform* myContext, AudioObjInstance* aoi)
{
  // fix up the pot and encoder values
  int iprt = floor(myContext->s.frequency.value.f - LOG_NOTE_A);
  float frac = myContext->s.frequency.value.f - iprt - LOG_NOTE_A;

  Serial.printf("freq is %f -> %f Hz\n",myContext->s.frequency.value.f,pow(2,myContext->s.frequency.value.f));
  
  enc0.setLimits(-3,12);
  if (frac > 0.5f)
  {
    frac -= 1.0f;
    iprt++;
  }
  enc0.setValue(iprt);

  ParamValue pv{frac};    
  HookControl(ctrl,1,freqLimits,pv); // frequency pot is #1: set hook

  Serial.printf("Hook set to %f; encoder to %d\n",pv.value.f,iprt);
}
  

template <> // template specialization for setting Waveform; needed for frequency setting
void updateFromControls<ContextWaveform>(ContextWaveform* myContext, AudioObjInstance* aoi)
{
  for (size_t i=0; i < myContext->paramCount; i++)
  {
    if (1 == i) // frequency
    {
      enc0.available();
      if (ScaleFreq(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
      {
        se->ShowValue(i);
        myContext->setParam(i,aoi);
      }      
    }
    else
    {
      if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
      {
        se->ShowValue(i);
        myContext->setParam(i,aoi);
      }
    }
  }
}

template <>
void processMIDIevent<ContextWaveform>(AudioObjInstance* aoi, MIDIevent* ev)
{
  ContextWaveform* ctxt = (ContextWaveform*) aoi->context;
  
  if (midi::NoteOn == ev->type) // note on
  {
    float freq;
    byte note = ev->note;
    
    note += 12*(ctxt->m.octave.value.i - 4);

    if (0 == ctxt->m.tuning.value.i) // magic: equal temperament
    {
      freq = PatcherVoice::noteToFreq(note);
      freq *= pow(2.0f,ctxt->m.detune.value.f);
      // TODO: add in current pitch bend here
    }
    else
    {
      note += (int) ctxt->m.detune.value.f; // just use another "tonewheel"
      freq = PatcherVoice::noteToFreq(note,notesHammond);
    }
    
    aoi->streamP.Waveform->frequency(freq);
    aoi->streamP.Waveform->amplitude(ev->velocity / 127.0f);
  }
}

  
int editWaveform(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthWaveform, ContextWaveform>(aoi,mode,params);  
}

//===========================================================================================
 const ParamChoice responsesBiquad[] = 
  {{"  (off) "   , 0},
     
   {"low pass"   , 1},
   {"high pass"  , 2},
   {"band pass"  , 3},
   {"notch"      , 4},
   {"low shelf"  , 5},
   {"high shelf" , 6},
   
#if defined(AUDIO_BIQUAD_HAS_PASSTHRU)      
   {"silent"     ,98},
   {"passthru"   ,99},
#endif // defined(AUDIO_BIQUAD_HAS_PASSTHRU)      
  };
  
const ParamEntry ContextBiquad::_params[] = {
  {"    stage", 0,3},
  {" response", PARAM_ENTRY_CHOICES(responsesBiquad)},
  {"frequency", 8.0f, 13.2877123795495f, 'l'},
  {"        Q", 0.0f,5.0f},
  {"     gain", 0.0f,2.0f},
  {"    slope", 0.0f,2.0f}  
};

static void (AudioFilterBiquad::* pFQ[])(uint32_t,float,float)
  {&AudioFilterBiquad::setLowpass,
   &AudioFilterBiquad::setHighpass,
   &AudioFilterBiquad::setBandpass,
   &AudioFilterBiquad::setNotch
  };

//static typeof(&AudioFilterBiquad::setLowShelf) pFGS[] = 
static void (AudioFilterBiquad::* pFGS[])(uint32_t,float,float,float)
  {&AudioFilterBiquad::setLowShelf,
   &AudioFilterBiquad::setHighShelf
  };

void ContextBiquad::setParam(int i, AudioObjInstance* aoi)
{
  int stge = s.stage.value.i;

  if (prevStage != stge // selected a different filter stage...
   && nullptr != se)    // ...and we're editing
  {
    for (size_t i=0; i < paramCount; i++) // display the values
    {
      if (0 != i) // not the stage number!
        stageSettings[prevStage][i] = aray[i].value; // store edited values
      aray[i].value = stageSettings[stge][i];      // copy new values
      se->ShowValue(i);  // display them
      if (0 != i)
        HookControl(ctrl,i,params[i],aray[i]);
    }
    prevStage = stge;
  }
  
  int resp = responsesBiquad[s.response.value.i].value;
  void (AudioFilterBiquad::* FQfn)(uint32_t,float,float);
  void (AudioFilterBiquad::* FGSfn)(uint32_t,float,float,float);

  switch (resp)
  {
    default:
    case 0:
      break;
#if defined(AUDIO_BIQUAD_HAS_PASSTHRU)      
    case 98: aoi->streamP.Biquad->setSilent(stge); break;  
    case 99: aoi->streamP.Biquad->setPassThru(stge); break;  
#endif // defined(AUDIO_BIQUAD_HAS_PASSTHRU)      

    case 1:
    case 2:
    case 3:
    case 4:
      FQfn = pFQ[resp-1];
      switch (i)
      {
        case 1:
        case 2:
        case 3: (aoi->streamP.Biquad->*FQfn)(stge,pow(2,s.frequency.value.f),s.Q.value.f); break;
      }
      break;
    
    case 5:
    case 6:
      FGSfn = pFGS[resp-5];
      switch (i)
      {
        case 1:
        case 2:
        case 4:
        case 5: (aoi->streamP.Biquad->*FGSfn)(stge,pow(2,s.frequency.value.f),s.gain.value.f,s.slope.value.f); break;
      }
      break;   
  } 
}

int editBiquad(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  int result = 0;
  switch (mode)
  {
    default: 
      result = editObjType<AudioFilterBiquad, ContextBiquad>(aoi,mode,params);
      break;

    case AudioEditMode::getParams:
      {
        ContextBiquad* myContext = (ContextBiquad*) aoi->context;
        getSetParams* orig = (getSetParams*) params;
        getSetParams working = *orig;
          
        for (int j=0;j<4;j++)
        {
          for (size_t i=0;i<myContext->paramCount;i++)
            myContext->aray[i].value = myContext->stageSettings[j][i];
          editGetParams<ContextBiquad>(aoi,&working);
          working.buffer   += working.sz;
          *working.buffer++ = ' ';
          *working.buffer   = 0;
          working.sz = orig->sz - working.sz - 1;
        }
        for (size_t i=0;i<myContext->paramCount;i++)
          myContext->aray[i].value = myContext->stageSettings[myContext->prevStage][i];
      }
      result = 1;
      break;

    case AudioEditMode::setParams:
      {
        ContextBiquad* myContext = (ContextBiquad*) aoi->context;
        getSetParams* orig = (getSetParams*) params;
        getSetParams working = *orig;
          
        for (int j=0;j<4;j++)
        {
          editSetParams<ContextBiquad>(aoi,&working);   // get set of values from passed string
          int jj = myContext->s.stage.value.i;          // defensive - stages could be out of order!
          for (size_t i=0;i<myContext->paramCount;i++)  // copy this set of values to stage settings
            myContext->stageSettings[jj][i] =  myContext->aray[i].value;
          working.buffer   += working.sz;         // point to next set of values
          working.sz = orig->sz - working.sz - 1; // and how many characters now remain
        }
        for (size_t i=0;i<myContext->paramCount;i++)
          myContext->aray[i].value = myContext->stageSettings[myContext->prevStage][i];
      }
      result = 1;
      break;     
  }
  
  return result;
}

//===========================================================================================
void ContextEnvelope::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Envelope->delay(pow(2,s.delay.value.f)); break;
    case 1: aoi->streamP.Envelope->attack(pow(2,s.attack.value.f)); break;
    case 2: aoi->streamP.Envelope->hold(pow(2,s.hold.value.f)); break;
    case 3: aoi->streamP.Envelope->decay(pow(2,s.decay.value.f)); break;
    case 4: aoi->streamP.Envelope->sustain(s.sustain.value.f); break;
    case 5: aoi->streamP.Envelope->release(pow(2,s.release.value.f)); break;
    case 6: aoi->streamP.Envelope->releaseNoteOn(pow(2,s.releaseNoteOn.value.f)); break;
  }
}

const ParamEntry ContextEnvelope::_params[7] = {
        {"    delay", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"   attack", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"     hold", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"    decay", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"  sustain", 0.0f, 1.0f}, 
        {"  release", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"relNoteOn", 0.0f, 13.2877123795495f, 'l'} // 1.0 .. 10,000.0ms
    };

int editEnvelope(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectEnvelope, ContextEnvelope>(aoi,mode,params);
}

template <>
void processMIDIevent<ContextEnvelope>(AudioObjInstance* aoi, MIDIevent* ev)
{
  if (midi::NoteOff == ev->type) // note off
    aoi->streamP.Envelope->noteOff();
  if (midi::NoteOn  == ev->type) // note on
    aoi->streamP.Envelope->noteOn();
}

// \return 0 for idle, 2 for active, 3 for sustain, should never be 3
template <>
int isActive<ContextEnvelope>(AudioObjInstance* aoi)
{
  return (aoi->streamP.Envelope->isSustain()?1:0)
       + (aoi->streamP.Envelope->isActive() ?2:0);
}
