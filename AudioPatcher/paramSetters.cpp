#include "objects.h"
#include "display.h"
#include "paramSetters.h"
#include "limitedEncoder.h"

#include <sf22aswt.h>
AudioSynthWavetable::instrument_data *wt_inst;
SF22ASWTreader sf22aswt;

extern M5w_8angle ctrl;
#define M5ANGLE_MIN   20
#define M5ANGLE_MAX 4080


/*
 * For some reason putting ParamEntry values in PROGMEM
 * worked briefly, then started giving Data Access violations
 * at boot. Startup trying to "initialise" when that's not
 * needed? Dunno.
 * 
 * TODO: figure out a way around this, if possible
 */
#undef PROGMEM
#define PROGMEM

//===========================================================================================
SettingsEditor* settingsEditor;
const float LOG_NOTE_A = 0.781359714f;           // frac(log2(440.0)) - 
const float MIDDLE_C = 16.3515978312875f*16.0f;  // middle C in Hz
/* PROGMEM constexpr */ ParamEntry freqLimits{nullptr,-1.0f,1.0f}; // special, for setting hook control

//===========================================================================================
size_t milliseconds2bytes(float ms) { return 2*AUDIO_SAMPLE_RATE_EXACT*ms/1000.0f; }

//===========================================================================================
FLASHMEM bool ScaleF(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter)
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


FLASHMEM bool ScaleFreq(const ParamEntry& pe, ParamValue& pv, int16_t raw, int16_t pow2, float filter)
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


FLASHMEM bool ScaleIminMax(int min, int max, ParamValue& pv, int16_t raw)
{
  bool result = false;
  int newVal = constrain(map(raw, M5ANGLE_MIN, M5ANGLE_MAX, min, max), min, max);
  if (newVal!= pv.value.i)
  {
    pv.value.i = newVal;
    result = true;
  }
  return result;
}


FLASHMEM bool ScaleI(const ParamEntry& pe, ParamValue& pv, int16_t raw)
{
  return ScaleIminMax(pe.min.i, pe.max.i, pv, raw);;
}


bool Scale(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter = 1.0f)
{
  bool result = false;
  switch(pe.ValType)
  { 
    case 'n':
    case 'l':
    case 'f': result = ScaleF(pe,pv,raw, filter); break;
    
    case 'c':
    case 'i': result = ScaleI(pe,pv,raw); break;

    case 'r': result = ScaleIminMax(1, pe.max.i, pv, raw); break;

    // string, arbitrary waveform, ...
    default: break;
  }
  return result;
}

FLASHMEM void HookControl(M5w_8angle& ctrl, int ch, const ParamEntry& pe, ParamValue& pv)
{
  switch (pe.ValType)
  {
    default: break; // catches string values

    case 'n':
    case 'l':
    case 'f': ctrl.setHook(ch, map(pv.value.f, pe.min.f, pe.max.f, M5ANGLE_MIN, M5ANGLE_MAX)); break;
    
    case 'c':
    case 'i':  ctrl.setHook(ch, map(pv.value.i, pe.min.i, pe.max.i, M5ANGLE_MIN, M5ANGLE_MAX)); break;

    case 'r':  ctrl.setHook(ch, map(pv.value.i, 1, pe.max.i, M5ANGLE_MIN, M5ANGLE_MAX)); break;
  }
}

FLASHMEM int testExit(uint32_t& exitAt)
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
FLASHMEM bool updateFromControls(AudioObjInstance* aoi)
{
  if (nullptr != settingsEditor)
  {
    for (size_t i=0; i < settingsEditor->paramCount; i++)
    {
      uint16_t pv = ctrl.getPot16(i);
      //if (7==i) // hardware issue with pot 7 :(
      //  Serial.println(pv);
      if (Scale(settingsEditor->params[i],settingsEditor->aray[i],pv,0.999f))
      {
        settingsEditor->ShowValue(i);
      }
    }
  }

  return false;
}

//===========================================================================================
FLASHMEM void SettingsEditor::Init(const char* title)
{
  display.ShowTitle(title,5,5);
  ShowPage();
}

PROGMEM uint16_t LEDbars[] =
  {
    CL(255,  0,  0),
    CL(224, 80,  0),
    CL(192,128,  0),
    CL(  0,192,  0),
    CL(  0, 96,128),
    CL( 32, 32,255),
    CL(192, 32,255),
    CL(255,255,255)  
  };
FLASHMEM void SettingsEditor::ShowPage(void)
{
  int row = 0, tmpLast = lastRowShown;
  size_t first = 0, nCtrl = paramCount;

  if (nullptr != pages)
  {
    first = pages[currentPage].start;
    nCtrl = pages[currentPage].count;
  }
  
  // Show the parameter rows / columns
  for (size_t i = 0; i < nCtrl; i++)
  {
    uint16_t LEDbar = i<8?LEDbars[i]:0;

    if (0 != params[i+first].xoff) /// if we have an X offset
      row--; // we're on the same row as before
    else
      BlankRow(row,27);      
    ShowLabel(i+first,row,5,27, LEDbar);
    ShowValue(i+first);
    if (!ctrl.isHooking(i) // specialized enterEditMode() may have hooked already - don't re-do
     && nullptr != params[i+first].label) // don't do for null parameter
      HookControl(ctrl,i,params[i+first],aray[i+first]);
    row++;            
  }
  lastRowShown = row; // last row with content on

  // Blank rows that were previously used
  while (row < tmpLast)
    BlankRow(row++,27);      
}

// Change currentPage to a new parameters page number, if possible
// Does not update screen
// \return true if valid page number
FLASHMEM bool SettingsEditor::ChangePage(int newPage)
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


FLASHMEM void SettingsEditor::ShowVoiceFlag(bool flag)
{
  display.ShowVoiceFlag(flag);
}

//===========================================================================================
FLASHMEM int editGetParamsAny(const ParamEntry* params, const ParamValue* aray, const size_t paramCount, getSetParams* p)
{
  size_t left = p->sz;
  char* ptr = p->buffer;
  int off = 0;
  
  for (size_t i=0; i < paramCount && off >= 0 && left >= 15; i++)
  {
    switch (params[i].ValType)
    {
      case 'n':
      case 'l':
      case 'f': off = sprintf(ptr,"%f,",aray[i].value.f); break;
      case 'c':
      case 'r':
      case 'i': off = sprintf(ptr,"%d,",aray[i].value.i); break;
      case 's': off = sprintf(ptr,"%s,",aray[i].value.s); break;
      case 't':
      case 'w': off = sprintf(ptr,"%s,",aray[i].value.w->path); break;
    }

    ptr += off;
    left -= off;            
  }
  p->sz = ptr - p->buffer; // amount of buffer used

  return paramCount != 0;
}

FLASHMEM int editSetParamsAny(const ParamEntry* params, ParamValue* aray, const size_t paramCount, getSetParams* p)
{
  char* ptr = p->buffer;
  int off = 0;
  
  for (size_t i=0; i < paramCount && off >= 0; i++)
  {
    ValUnion value;
    
    switch (params[i].ValType)
    {
      // floating point value
      case 'n':
      case 'f':
      case 'l':
        sscanf(ptr,"%f,%n",&value.f,&off);
        if (value.f < params[i].min.f || value.f > params[i].max.f)
          value.f = (params[i].min.f + params[i].max.f) / 2.0f;
        Serial.printf("%s = %.3f ... ",params[i].label,value.f);
        aray[i].value.f = value.f;
        break;
        
      // integer or list selection
      case 'i':
      case 'c':
        sscanf(ptr,"%d,%n",&value.i,&off);
        if (value.i < params[i].min.i || value.i > params[i].max.i)
          value.i = (params[i].min.i + params[i].max.i) / 2;
        Serial.printf("%s = %d ... ",params[i].label,value.i);
        aray[i].value.i = value.i;
        break;
      
      // reciprocal
      case 'r':
        sscanf(ptr,"%d,%n",&value.i,&off);
        if (value.i < 1 || value.i > params[i].max.i)
          value.i = (1 + params[i].max.i) / 2;
        Serial.printf("%s = %d ... ",params[i].label,value.i);
        aray[i].value.i = value.i;
        break;

      // string or arbitrary waveform (*arbWAVrecord)
      case 's':
      case 't':
      case 'w':
        off = 0; // skip parameter if we fail (This Never Happens)
        do 
        {
          char* mem, *path;
          size_t spaceNeeded;

          // find comma that terminates the string
          char* comma = strchr(ptr,',');
          if (nullptr == comma) // oh heck - now what?
            break;

          // remove leading spaces
          while (' ' == *ptr && ptr != comma)
            ptr++;

          // allocate space to store it
          if ('s' != params[i].ValType)
          {
            mem = aray[i].value.w->prepare(comma - ptr);
            path = aray[i].value.w->path;
          }
          else
          {            
            spaceNeeded = comma - ptr + 1;
            spaceNeeded = (spaceNeeded + 16 ) & ~16;
            mem = (char*) malloc(spaceNeeded); // just enough space
            path = mem;
          }
          if (nullptr == mem)
            break;
          
          // scan string into buffer
          //sscanf(ptr,"%s,%n",mem + extra,&off);
          memcpy(path,ptr,comma-ptr); // sscanf can't do this job ...
          path[comma-ptr] = 0; // .. do it ourselves, with terminator...
          off = comma-ptr+1;

          if ('s' != params[i].ValType)
          {
            // it's a path to an arbitrary waveform, which
            // hasn't yet been loaded in
            aray[i].value.w->loaded = false;
            Serial.printf("%s = <%s> ... ",params[i].label,aray[i].value.w->path);
          }
          else
          {
            value.s = mem;
            Serial.printf("%s = %s ... ",params[i].label,value.s);
            aray[i].value.s = value.s;
          }
          Serial.flush();
        } while (0);
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
FLASHMEM int editSetStreamParams(AudioObjInstance& aoi)
{
  int result = 0;
  ContextBase* myContext = (ContextBase*) aoi.context;

  if (nullptr != myContext)
  {  
    // set the actual stream object's parameters
    for (size_t i=0; i < myContext->paramCount; i++)
      if (myContext->params[i].ValType != 'n') // unless the flag says not to...
        myContext->setParam(i,&aoi);        // ...set the value
  
    result = (int) myContext->paramCount;
  }
  
  return result;  
}


//=====================================================================================
FLASHMEM void ContextBase::copyParamsTo(ContextBase& dst)
{
  memcpy(dst.aray, aray, paramCount * sizeof aray[0]);
  memcpy(dst.MIDIvalues, MIDIvalues, MIDIparamCount * sizeof MIDIvalues[0]);
}

FLASHMEM void CopyContext(void* src, void* dst)
{
  // some objects don't need a context, e.g. AudioEffectMultiply
  if (nullptr != src && nullptr != dst)
  {
    ((ContextBase*) src)->copyParamsTo(*(ContextBase*) dst);
  }
}

//===========================================================================================
// Strong definitions of setup controls
//===========================================================================================
// Some object types should only ever be constructed:
FLASHMEM int editInputI2S(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  if (AudioEditMode::constructor == mode)
    editCreateStream<AudioInputI2S>(aoi,nullptr); 
  return 0;
}

FLASHMEM int editOutputI2S(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  if (AudioEditMode::constructor == mode)
    editCreateStream<AudioOutputI2S>(aoi,nullptr); 
  return 0;    
}


//===========================================================================================
FLASHMEM void ContextBitcrusher::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Bitcrusher->bits(s.bits.value.i); break;
    case 1: aoi->streamP.Bitcrusher->sampleRate(params[i].min.f / s.sampleRate.value.i); break;
  }
}

PROGMEM constexpr ParamEntry ContextBitcrusher::_params[2] = 
{
  {"       bits", 1, 16},
  {"sample rate", AUDIO_SAMPLE_RATE, 50},
};


FLASHMEM int editBitcrusher(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectBitcrusher, ContextBitcrusher>(aoi,mode,params);    
}

//===========================================================================================
FLASHMEM void ContextChorus::allocMem(memRecord& mem, size_t newsize, AudioObjInstance* aoi)
{
  if (nullptr != mem.ptr)
    free(mem.ptr);
  mem.ptr = (short*) malloc(newsize);
  mem.sz  = newsize;
  memset(mem.ptr,0,mem.sz);  
  aoi->streamP.Chorus->begin(mem.ptr,milliseconds2bytes(s.length.value.f)/2,s.voices.value.i);  
  AudioInterrupts();  
}

FLASHMEM void ContextChorus::setParam(int i, AudioObjInstance* aoi)
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

PROGMEM constexpr ParamEntry ContextChorus::_params[2] = 
{
  {"length [ms]", 10.0f, 200.0f},
  {"     voices", 1, 20},
};

FLASHMEM void ContextChorus::enterEditMode(AudioObjInstance* aoi)
{
  tmp = mem;
  AudioNoInterrupts();
  mem.ptr = nullptr; // prevent freeing old chorus memory for now
  allocMem(mem,milliseconds2bytes(params[0].max.f),aoi);
}

template <> FLASHMEM
void enterEditMode<ContextChorus>(ContextChorus* myContext, AudioObjInstance* aoi)
{
  myContext->enterEditMode(aoi);
}

FLASHMEM void ContextChorus::exitEditMode(AudioObjInstance* aoi)
{
  if (milliseconds2bytes(s.length.value.f) <= tmp.sz) // old allocation is OK for new setting
  {
    aoi->streamP.Chorus->begin(tmp.ptr,milliseconds2bytes(s.length.value.f),s.voices.value.i);
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

template <> FLASHMEM
void exitEditMode<ContextChorus>(ContextChorus* myContext, AudioObjInstance* aoi)
{
  myContext->exitEditMode(aoi);
}

FLASHMEM int editChorus(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectChorus, ContextChorus>(aoi,mode,params); 
}

//===========================================================================================
FLASHMEM void ContextDelayExternal::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    default: break;
    case 0 ... 7: // tap positions
    {
      float secs = s.taps[i].value.f;

      if (secs > 0.0f)
        aoi->streamP.DelayExternal->delay(i, secs * 1000.0f); 
      else        
        aoi->streamP.DelayExternal->disable(i); 
    } break;
    //case 1: aoi->streamP.Bitcrusher->sampleRate(params[i].min.f / s.sampleRate.value.i); break;
  }
}

const ParamChoice delayMemTypes[] 
  {{"ExtMem", 0},
  {"PSRAM", 1},
  {"Heap", 2},
};

const ParamPage ContextDelayExternal::_pages[2] {{0,8},{8,2}};

PROGMEM constexpr ParamEntry ContextDelayExternal::_params[10] = 
{
  {"tap1", -0.05f, 1.0f}, {"tap2", -0.05f, 1.0f, EDIT_DELAY_EXTERNAL_OFF},
  {"tap3", -0.05f, 1.0f}, {"tap4", -0.05f, 1.0f, EDIT_DELAY_EXTERNAL_OFF},
  {"tap5", -0.05f, 1.0f}, {"tap6", -0.05f, 1.0f, EDIT_DELAY_EXTERNAL_OFF},
  {"tap7", -0.05f, 1.0f}, {"tap8", -0.05f, 1.0f, EDIT_DELAY_EXTERNAL_OFF},

  {"max time", -4.0f, 5.0f, 'l'}, // max delay time
  {"location", PARAM_ENTRY_CHOICES(delayMemTypes)} 
};


FLASHMEM int editDelayExternal(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectDelayExternal, ContextDelayExternal>(aoi,mode,params);    
}

template <>
// Template specialization for creating a new 
//                             VVVV
void editCreateStream<AudioEffectDelayExternal>(AudioObjInstance* aoi, AudioObjInstance* original)
{ 
  aoi->streamP.DelayExternal = new AudioEffectDelayExternal{AudioEffectDelayExternal_CONSTRUCTOR}; 
}


//===========================================================================================
/// TODO: see if we can make much of this code common with the Chorus effect
FLASHMEM void ContextFlange::allocMem(memRecord& mem, size_t newsize, AudioObjInstance* aoi)
{
  if (nullptr != mem.ptr)
    free(mem.ptr);
  mem.ptr = (short*) malloc(newsize); // possibly more than needed
  mem.sz  = newsize;
  memset(mem.ptr,0,mem.sz);

  size_t bufferSamples = milliseconds2bytes(s.length.value.f) / 2;
  aoi->streamP.Flange->begin(mem.ptr, bufferSamples*2,
                             s.offset.value.f * bufferSamples,
                             s.depth.value.f * bufferSamples,
                             pow(2.0f,s.rate.value.f));
  AudioInterrupts();  
}

FLASHMEM void ContextFlange::setParam(int i, AudioObjInstance* aoi)
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
          aoi->streamP.Flange->begin(mem.ptr, newsize, // not using all buffer, maybe
                                    s.offset.value.f * newsize/2,
                                    s.depth.value.f * newsize/2,
                                    pow(2.0f,s.rate.value.f));
      }
      break;
      
    case 1 ... 3: 
      {
        size_t bufferSamples = milliseconds2bytes(s.length.value.f) / 2;

        aoi->streamP.Flange->voices(s.offset.value.f * bufferSamples,
                                    s.depth.value.f * bufferSamples,
                                    pow(2.0f,s.rate.value.f));
      }
      break;
  }
}

PROGMEM constexpr ParamEntry ContextFlange::_params[4] = 
{
  {"length [ms]", 10.0f, 200.0f},
  {"     offset", 0.0f,  1.0f},
  {"      depth", 0.0f,  0.5f},
  {"       rate",   -5.0f,  4.0f, 'l'}, // 0.03 .. 16Hz
};

FLASHMEM void ContextFlange::enterEditMode(AudioObjInstance* aoi)
{
  tmp = mem;
  AudioNoInterrupts();
  mem.ptr = nullptr; // prevent freeing old flange memory for now
  allocMem(mem,milliseconds2bytes(params[0].max.f),aoi);
}

template <> FLASHMEM
void enterEditMode<ContextFlange>(ContextFlange* myContext, AudioObjInstance* aoi)
{
  myContext->enterEditMode(aoi);
}

FLASHMEM void ContextFlange::exitEditMode(AudioObjInstance* aoi)
{
  if (milliseconds2bytes(s.length.value.f) <= tmp.sz) // old allocation is OK for new setting
  {
    size_t newsize = milliseconds2bytes(s.length.value.f);
    
    aoi->streamP.Flange->begin(tmp.ptr, newsize, 
                          s.offset.value.f * newsize/2,
                          s.depth.value.f * newsize/2,
                          pow(2.0f,s.rate.value.f));
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

template <> FLASHMEM
void exitEditMode<ContextFlange>(ContextFlange* myContext, AudioObjInstance* aoi)
{
  myContext->exitEditMode(aoi);
}

FLASHMEM int editFlange(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectFlange, ContextFlange>(aoi,mode,params); 
}

//===========================================================================================
FLASHMEM void ContextFreeverb::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Freeverb->roomsize(s.roomsize.value.f); break;
    case 1: aoi->streamP.Freeverb->damping(s.damping.value.f); break;
  }
}

PROGMEM constexpr ParamEntry ContextFreeverb::_params[2] = 
{
  {"room size", 0.0f, 1.0f},
  {"  damping", 0.0f, 1.0f},
};


FLASHMEM int editFreeverb(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectFreeverb, ContextFreeverb>(aoi,mode,params);    
}

//===========================================================================================
FLASHMEM void ContextFreeverbStereo::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.FreeverbStereo->roomsize(s.roomsize.value.f); break;
    case 1: aoi->streamP.FreeverbStereo->damping(s.damping.value.f); break;
  }
}

PROGMEM constexpr ParamEntry ContextFreeverbStereo::_params[2] = 
{
  {"room size", 0.0f, 1.0f},
  {"  damping", 0.0f, 1.0f},
};


FLASHMEM int editFreeverbStereo(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectFreeverbStereo, ContextFreeverbStereo>(aoi,mode,params);    
}

//===========================================================================================
FLASHMEM void ContextReverb::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Reverb->reverbTime(s.reverbTime.value.f); break;
  }
}

PROGMEM constexpr ParamEntry ContextReverb::_params[1] = 
{
  {"reverb time", 0.0f, 10.0f},
};


FLASHMEM int editReverb(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectReverb, ContextReverb>(aoi,mode,params);    
}

//===========================================================================================
FLASHMEM void ContextMixer4::setParam(int i, AudioObjInstance* aoi)
{
  aoi->streamP.Mixer4->gain(i,gains[i].value.f); 
}

PROGMEM constexpr ParamEntry ContextMixer4::_params[4] = 
{
  {"ch1", 0.0f, 1.0f},
  {"ch2", 0.0f, 1.0f},
  {"ch3", 0.0f, 1.0f},
  {"ch4", 0.0f, 1.0f},
};

PROGMEM constexpr ParamEntry ContextMixer4::MIDIparams[4] = 
{
  {"CC1", -1, 119}, 
  {"CC2", -1, 119}, 
  {"CC3", -1, 119}, 
  {"CC4", -1, 119}  
};


FLASHMEM int editMixer4(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioMixer4, ContextMixer4>(aoi,mode,params);    
}

template <> FLASHMEM
void processMIDIevent<ContextMixer4>(AudioObjInstance* aoi, MIDIevent* ev)
{
  ContextMixer4* ctxt = (ContextMixer4*) aoi->context;
  
  switch (ev->type)
  {
    case midi::ControlChange:
    {
      for (int i=0;i<4;i++)
        if (ev->CCnum == ctxt->CCs[i].value.i)
        {
          float ampl = ev->CCval / 127.0f;// map((float) ev->CCval,0.0f,127.0f,ctxt->m.CCmin.value.f,ctxt->m.CCmax.value.f);
          aoi->streamP.Mixer4->gain(i,ev->CCval / 127.0f);
          ctxt->gains[i] = ampl;
        }
    }
    break;
  }
}


//===========================================================================================
const ParamChoice panLawTypes[] 
  {{"analogue", 0},
   {"DAW", 1},
  };

FLASHMEM void ContextMixerStereo::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    default:
      break;
      
    case 0 ... 15:
      {
        int ch = i / 2;
        if (0 == (i & 1))
          aoi->streamP.MixerStereo->gain(ch,gainOrPan[i].value.f); 
        else
          aoi->streamP.MixerStereo->pan(ch,gainOrPan[i].value.f); 
      }
      break;

    case 16: // master gain
      aoi->streamP.MixerStereo->gain(gainOrPan[i].value.f);
      for (int j=0; j < 8; j++)
        gainOrPan[j*2].value.f = gainOrPan[i].value.f;
      break;

    case 17: // soft knee behaviour
      aoi->streamP.MixerStereo->setSoftKnee(gainOrPan[i].value.f);
      break;
      
    case 18: // pan law
      aoi->streamP.MixerStereo->setPanLaw(gainOrPan[i].value.f);
      break;
      
    case 19: // pan type
      switch (panLawTypes[gainOrPan[i].value.i].value)
      {
        case 0:
          aoi->streamP.MixerStereo->setPanType(AudioMixerStereo::PanLawType::analogue);
          break;
        case 1:
          aoi->streamP.MixerStereo->setPanType(AudioMixerStereo::PanLawType::DAW);
          break;
      }
      break;
  }
  
}

const ParamPage ContextMixerStereo::_pages[3] {{0,8},{8,8},{16,4}};

PROGMEM constexpr ParamEntry ContextMixerStereo::_params[20] = 
{
  {"ch1", 0.0f, 1.0f}, {"pan1", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch2", 0.0f, 1.0f}, {"pan2", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch3", 0.0f, 1.0f}, {"pan3", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch4", 0.0f, 1.0f}, {"pan4", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch5", 0.0f, 1.0f}, {"pan5", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch6", 0.0f, 1.0f}, {"pan6", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch7", 0.0f, 1.0f}, {"pan7", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch8", 0.0f, 1.0f}, {"pan8", -1.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"     gain", 0.0f, 1.0f, 'n'}, // 
  {"soft knee", 0.0f, 1.0f},
  {"  pan law", 0.01f, 1.0f},
  { "pan type", PARAM_ENTRY_CHOICES(panLawTypes)} 
  
};

template <> FLASHMEM
// Template specialization for creating a new 
//                             VVVV
void editCreateStream<AudioMixerStereo>(AudioObjInstance* aoi, AudioObjInstance* original){ aoi->streamP.MixerStereo = new AudioMixerStereo{AudioMixerStereo_CONSTRUCTOR}; }

FLASHMEM int editMixerStereo(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioMixerStereo, ContextMixerStereo>(aoi,mode,params);    
}


//===========================================================================================
FLASHMEM void ContextMixer::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    default:
      break;
      
    case 0 ... 7:
      aoi->streamP.Mixer->gain(i,gainEtc[i].value.f); 
      break;

    case 8: // master gain
      aoi->streamP.Mixer->gain(gainEtc[i].value.f);
      for (int j=0; j < 8; j++)
        gainEtc[j].value.f = gainEtc[i].value.f;
      break;

    case 9: // soft knee behaviour
      aoi->streamP.MixerStereo->setSoftKnee(gainEtc[i].value.f);
      break;      
  }
  
}

const ParamPage ContextMixer::_pages[2] {{0,8},{8,2}};

PROGMEM constexpr ParamEntry ContextMixer::_params[10] = 
{
  {"ch1", 0.0f, 1.0f}, 
  {"ch2", 0.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  {"ch3", 0.0f, 1.0f}, 
  {"ch4", 0.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF}, 
  {"ch5", 0.0f, 1.0f},
  {"ch6", 0.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF}, 
  {"ch7", 0.0f, 1.0f}, 
  {"ch8", 0.0f, 1.0f, EDIT_MIXER_STEREO_PAN_OFF},
  
  {"     gain", 0.0f, 1.0f, 'n'}, // 
  {"soft knee", 0.0f, 1.0f}
};


template <> FLASHMEM
// Template specialization for creating a new 
//                                 VVVV
void editCreateStream<AudioMixer>(AudioObjInstance* aoi, AudioObjInstance* original){ aoi->streamP.Mixer = new AudioMixer{AudioMixer_CONSTRUCTOR}; }

FLASHMEM int editMixer(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioMixer, ContextMixer>(aoi,mode,params);    
}




//======================================================================
//======================================================================
//
//                   888      888       888        d8888 888     888 
//                   888      888   o   888       d88888 888     888 
//                   888      888  d8b  888      d88P888 888     888 
//   8888b.  888d888 88888b.  888 d888b 888     d88P 888 Y88b   d88P 
//      "88b 888P"   888 "88b 888d88888b888    d88P  888  Y88b d88P  
//  .d888888 888     888  888 88888P Y88888   d88P   888   Y88o88P   
//  888  888 888     888 d88P 8888P   Y8888  d8888888888    Y888P    
//  "Y888888 888     88888P"  888P     Y888 d88P     888     Y8P     
//  
//======================================================================
template<> FLASHMEM bool arbWAVrecord<int16_t>::isDefault(void) {return sampleData == arbWAV_sax; }

/*
 * Load arbitrary waveform using given complete path
 */
template<> FLASHMEM
FLASHMEM bool arbWAVrecord<int16_t>::load(const char* buf)
{
  bool result = false;
  int16_t tmp[256]; // temporary space for the waveform
  size_t spaceNeeded = sizeof tmp + strlen(buf) + 1;
  File f;

  //Serial.printf("Load %s\n",buf); Serial.flush();

  spaceNeeded = (spaceNeeded + 16) & ~16; // round up a bit
  do
  {
    int16_t* mem = sampleData;
    char* path;

    // open the file
    f = SD.open(buf,FILE_READ);
    if (!f) break;

    // read the file
    if (sizeof tmp != f.read(tmp, sizeof tmp)) 
      break;

    // allocate storage if necessary
    if (arbWAV_sax == sampleData // using default...
      || recSize < spaceNeeded)  // ...or not enough space
    {        
      mem = (int16_t*) malloc(spaceNeeded);
      if (nullptr == mem)
        break;
      reset();
    }
    path = (char*) mem + sizeof tmp;

    // copy the data and point to it
    memcpy(mem,tmp,sizeof tmp); // get sample data
    strcpy(path, buf);          // and where it was loaded from
    setAll(mem, path, spaceNeeded, true);
    result = true;
    
  } while (0);

  if (f)
    f.close();
  
  return result;
}


/*
  Reset arbitrary waveform to safe value, and 
  free the memory it's using.
*/
template<> FLASHMEM
FLASHMEM void arbWAVrecord<int16_t>::reset(void)
{
  int16_t* oldArbWAV = sampleData;

  if (arbWAV_sax != oldArbWAV)
  {
    setAll((int16_t*) arbWAV_sax,(char*) "/<sax>.",0,true); // reset to safe default
    free((void*) oldArbWAV);  // free the memory
  }
}


/*
  Prepare memory to store waveform and its source path
  \return pointer to memory; may be nullptr
*/
template<> FLASHMEM
FLASHMEM char* arbWAVrecord<int16_t>::prepare(size_t pathLen)
{
  // allocate space to store it
  char* mem = (char*) sampleData; // assume we can use what we have
  size_t spaceNeeded = ARB_WAV_SAMPLES*sizeof *sampleData + pathLen + 1;
  spaceNeeded = (spaceNeeded + 16 ) & ~16;

  if (isDefault() || spaceNeeded > recSize)
  {
    mem = (char*) malloc(spaceNeeded); // just enough space
    if (!isDefault())   // old space was allocated...
      free(sampleData); // ...so free it

    // update to show new allocation size
    if (nullptr != mem)
    {
      sampleData = (int16_t*) mem;
      path = (char*) (sampleData+ARB_WAV_SAMPLES);
      *path = 0;
      recSize = spaceNeeded;
    }
    else
    {
      sampleData = nullptr;
      path = nullptr;
      recSize = 0;      
    }
    loaded = false;
  }

  return mem;
}

//======================================================================
/*
 * Note the following are NOT template specializations:
 * the class has already been specialized, so these are
 * straight class methods.
 */
bool arbWAVrecord<AudioSynthWavetable::instrument_data>::isDefault(void) {return sampleData == &Harp; }
/*
 * Load wavetable instrument using given complete path
 */
FLASHMEM bool arbWAVrecord<AudioSynthWavetable::instrument_data>::load(const char* buf)
{
  bool result = false;
  size_t spaceNeeded = strlen(buf) + 1;
  File f;

  //Serial.printf("Load %s\n",buf); Serial.flush();

  spaceNeeded = (spaceNeeded + 16) & ~16; // round up a bit
  do
  {
    char* filePath = path;

    // allocate storage if necessary
    if (recSize < spaceNeeded)  // not enough space
    {        
      filePath = (char*) malloc(spaceNeeded);
      if (nullptr == filePath)
        break;
      reset();
    }
    path = filePath;

    // copy the data and point to it
    //memcpy(mem,tmp,sizeof tmp); // get sample data
    strcpy(path, buf);          // and where it was loaded from
    setAll(nullptr, path, spaceNeeded, false, getIndex());

    result = true;
    
  } while (0);
  
  return result;
}

FLASHMEM void arbWAVrecord<AudioSynthWavetable::instrument_data>::instListAddEntry(SF22ASWT::inst_rec& inst)
{
  instEntryPtr item{new instEntry(idx++,String(inst.achInstName))};
  //item->index = idx++;
  //item->name = new String(inst.achInstName);
  instList.push_back(item);
  //Serial.printf("At %08X: #%d: %s\n", (uint32_t) item.p, item.p->index, item.p->name.c_str()); Serial.flush();
}


FLASHMEM static void instListAddEntry(SF22ASWT::inst_rec& inst, void* params)
{
  arbWAVrecord<AudioSynthWavetable::instrument_data>* id = (arbWAVrecord<AudioSynthWavetable::instrument_data>*) params;
  id->instListAddEntry(inst);
}


FLASHMEM bool operator<(const instEntryPtr& lhs, const instEntryPtr& rhs )
{ 
  return lhs.p->name < rhs.p->name; 
}


FLASHMEM void arbWAVrecord<AudioSynthWavetable::instrument_data>::getInstrumentList(void)
{
  sf22aswt.ReadFile(path);
  idx = 0;
  sf22aswt.ProcessInstrumentList(::instListAddEntry,this);
  //Serial.println();
  std::stable_sort(instList.begin(),instList.end());
  /*
  for (auto e : instList)
  {
    Serial.printf("#%d: %s\n", e.p->index, e.p->name.c_str());
  }
    */
}

FLASHMEM void arbWAVrecord<AudioSynthWavetable::instrument_data>::emptyInstrumentList(void)
{
  while (instList.size() > 0)
  {
    instEntryPtr entry = instList.back();
    //Serial.printf("Delete %08X\n", (uint32_t) entry.p); Serial.flush();
    delete entry.p;
    instList.pop_back();
  }
}

/*
  Reset wavetable instrument to safe value, and 
  free the memory it's using.
*/
FLASHMEM void arbWAVrecord<AudioSynthWavetable::instrument_data>::reset(void)
{
  AudioSynthWavetable::instrument_data* oldArbWAV = sampleData;

  if (&Harp != oldArbWAV)
  {
    free(path);
    setAll((AudioSynthWavetable::instrument_data*) &Harp, // reset to safe default
            (char*) "/<Harp>.",0,true,-1);
    //free((void*) oldArbWAV);  // free the memory
  }
}

/*
  Prepare memory to store wavetable instrument and its source path
  \return pointer to memory; may be nullptr
*/
FLASHMEM char* arbWAVrecord<AudioSynthWavetable::instrument_data>::prepare(size_t pathLen)
{
  // allocate space to store it
  char* mem = (char*) path; // assume we can use what we have
  size_t spaceNeeded = pathLen + 1;
  spaceNeeded = (spaceNeeded + 16 ) & ~16;

  if (isDefault() || spaceNeeded > recSize)
  {
    mem = (char*) malloc(spaceNeeded); // just enough space
    // if (!isDefault())   // old space was allocated...
    //  free(sampleData); // ...so free it

    // update to show new allocation size
    if (nullptr != mem)
    {
      sampleData = nullptr;
      path = mem;
      *path = 0;
      recSize = spaceNeeded;
    }
    else
    {
      sampleData = nullptr;
      path = nullptr;
      recSize = 0;      
    }
    loaded = false;
  }

  return mem;
}

void InstrumentPicker::createFileList(const char* path, mode_e mode)
{
  context.arbWAV.getInstrumentList();
  for (auto e : context.arbWAV.instList)
    fileList.push_back({e.p->name,false,e.p->index});
}

void InstrumentPicker::clearFileList(void)
{
  context.arbWAV.emptyInstrumentList();
  fileList.clear();
}

void InstrumentPicker::loadInstrument(const char* nme)
{
  arbWAVrecord<AudioSynthWavetable::instrument_data>& arb = context.arbWAV;
  arb.setIndex(fileList.at(enc1.getValue()).index); // cheat a bit here...

  // Serial.printf("InstrumentPicker::load(%s) - index %d\n", nme, arb.getIndex());
  context.arbWAVloaded = arb.loadInstrument();

  /*
  if (context.arbWAVloaded)
    Serial.printf("Loaded to %08X\n", (uint32_t) arb.sampleData);
  else
    Serial.println("Failed");
  Serial.flush();    
  */
}


//======================================================================
//======================================================================


//===========================================================================================
const ParamChoice velocityShapes[] 
  {{"linear", 0},
   {"curved", 1},
   {"as set", 2},
   {"max", 3}
  };

const ParamChoice tuningTypes[] 
  {{"equal", 0},
   {"Hammond", 1},
  };

PROGMEM constexpr ParamEntry ContextWaveformModulated::MIDIparams[] 
{
  {"   octave", 0, 9}, // middle C = 261.63Hz = note#60 = octave 4
  {"   detune", -6.00f, +6.00f}, // semitones / cents
  {" velocity",PARAM_ENTRY_CHOICES(velocityShapes)},
  {"   tuning",PARAM_ENTRY_CHOICES(tuningTypes)},
  {"PB amount",0.0f, 12.0f},
};

const ParamChoice waveShapes[13] = 
  {{"sine",0},
   {"saw" , 1},
   {"square" , 2},
   {"triangle" , 3},
   {"pulse" , 5},
   {"saw_rev" , 6},
   {"s&h" , 7},
   {"tri_var" , 8},
   {"saw_bl" , 9},
   {"saw_rev_bl" , 10},
   {"square_bl" , 11},
   {"pulse_bl" , 12},
   {"arbitrary" , 4}
  };
  
ParamChoice modTypes[] = {{"frequency",0},{"phase",1}};


PROGMEM constexpr ParamEntry ContextWaveformModulated::_params[7] = 
{
  {" waveform", PARAM_ENTRY_CHOICES(waveShapes)},
  {"frequency", -4.0f, 14.0f, 'l'}, // log2(freq) is what we actually store
  {"amplitude", 0.0f, 1.0f},
  {"   offset", -1.0f, 1.0f},
  {" mod type",PARAM_ENTRY_CHOICES(modTypes)},
  {"mod depth",0.0f, 100.0f}, // use 0-100%; frequency is 0.0 - 12.0 octaves; phase modulation could be 9000°
  {" arb. WAV", 'w'}
};

FLASHMEM void ContextWaveformModulated::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.WaveformModulated->begin(waveShapes[s.waveform.value.i].value); break;
    case 1: 
      noteFreq = pow(2,s.frequency.value.f);
      aoi->streamP.WaveformModulated->frequency(noteFreq); break;
    case 2: aoi->streamP.WaveformModulated->amplitude(s.amplitude.value.f); break;
    case 3: aoi->streamP.WaveformModulated->offset(s.offset.value.f); break;
    
    case 4: 
    case 5: 
      switch (modTypes[s.modType.value.i].value)
      {
        case 0: // frequency modulation
          aoi->streamP.WaveformModulated->frequencyModulation(s.modDepth.value.f * 0.12f); 
          break;
        case 1: // frequency modulation
          aoi->streamP.WaveformModulated->phaseModulation(s.modDepth.value.f * s.modDepth.value.f * 0.9f); 
          break;
      }
      break;

    case 7:
    case ARBWAV_PARAM:
    {
      arbWAVrecord<int16_t>& arb = *s.arbWAV.value.w;

      if (arb.loadIfNeeded())
        aoi->streamP.WaveformModulated->arbitraryWaveform(arb.sampleData,10000.0f);
    }
      break;
  }
}


template <> FLASHMEM
void enterEditMode<ContextWaveformModulated>(ContextWaveformModulated* myContext, AudioObjInstance* aoi)
{
  // fix up the pot and encoder values
  int iprt = floor(myContext->s.frequency.value.f - LOG_NOTE_A);
  float frac = myContext->s.frequency.value.f - iprt - LOG_NOTE_A;

  // Serial.printf("freq is %f -> %f Hz\n",myContext->s.frequency.value.f,pow(2,myContext->s.frequency.value.f));
  
  enc0.setLimits(-3,12);
  if (frac > 0.5f)
  {
    frac -= 1.0f;
    iprt++;
  }
  enc0.setValue(iprt);

  ParamValue pv{frac};    
  HookControl(ctrl,1,freqLimits,pv); // frequency pot is #1: set hook

  enc0.setLED(LimitedEncoder::colour(1));
  enc5.setLED(LimitedEncoder::colour(6));

  // Serial.printf("Hook set to %f; encoder to %d\n",pv.value.f,iprt);
}

template <> FLASHMEM
void exitEditMode<ContextWaveformModulated>(ContextWaveformModulated* myContext, AudioObjInstance* aoi)
{
  enc0.setLED(0);
  enc5.setLED(0);
}

template<class Tctxt>
bool selectArbWAVfile(Tctxt* myContext, AudioObjInstance* aoi)
{
  bool result = false;

  if (nullptr != myContext->fileSelector)
  {
    static uint32_t exitAt = 0; // flag to track "exit" encoder button state
    int keepChoosing = testExit(exitAt);

    myContext->fileSelector->edit();

    if (myContext->arbWAVloaded || !keepChoosing) // user selected a different wave, or quit...
    {
      if (myContext->arbWAVloaded)
        myContext->setParam(ContextWaveformModulated::ARBWAV_PARAM,aoi);  // ...tell the object about it
      myContext->fileSelector->exit();
      delete myContext->fileSelector;
      myContext->fileSelector = nullptr;

      // fix up the display
      settingsEditor->InitArea();
      settingsEditor->Init(aoi->objP->name);
      // restore settings and encoder limits
      enterEditMode(myContext,aoi);
    }
    result = true; // file selector is or was active
  }

  return result;
}


template<class Tctxt>
bool selectInstrument(Tctxt* myContext, AudioObjInstance* aoi)
{
  bool result = false;

  if (nullptr != myContext->instSelector)
  {
    static uint32_t exitAt = 0; // flag to track "exit" encoder button state
    int keepChoosing = testExit(exitAt);

    myContext->instSelector->edit();

    if (myContext->arbWAVloaded || !keepChoosing) // user selected a different wave, or quit...
    {
      if (myContext->arbWAVloaded)
        myContext->setParam(ContextWaveformModulated::WTINST_PARAM,aoi);  // ...tell the object about it
      myContext->instSelector->exit();
      delete myContext->instSelector;
      myContext->instSelector = nullptr;

      //myContext->s.index.value.i = myContext->arbWAV.index;

      // fix up the display
      settingsEditor->InitArea();
      settingsEditor->Init(aoi->objP->name);
      // restore settings and encoder limits
      enterEditMode(myContext,aoi);
    }
    result = true; // file selector is or was active
  }

  return result;
}

/*
 * Poll encoder button used to enter arbitrary waveform file
 * selection dialogue
 */
template<class Tctxt>
bool pollFileSelect(Tctxt* myContext, LimitedEncoder& enc)
{
  bool result = false;

  if (enc.wasClicked())
  {
    myContext->fileSelector = new FileLoader(enc0,enc1,enc2,
                                        settingsEditor->display,
                                        myContext->root, myContext->extn, 
                                        FileBase::mode_e::load,
                                        *myContext);
    myContext->fileSelector->enter(false); // area is already saved, don't repeat that
    myContext->arbWAVloaded = false;
    result = true;
  }
  
  return result;
}

/*
 * Poll encoder button used to enter arbitrary waveform file
 * selection dialogue
 */
bool pollInstSelect(ContextWavetable* myContext, LimitedEncoder& enc)
{
  bool result = false;

  if (enc.wasClicked())
  {
    myContext->instSelector = new InstrumentPicker(enc0,enc1,enc2,
                                        settingsEditor->display,
                                        *myContext);
    myContext->instSelector->enter(false); // area is already saved, don't repeat that
    myContext->arbWAVloaded = false;
    result = true;
  }
  
  return result;
}


// template specialization for setting WaveformModulated; needed for frequency setting
// and arbitrary waveform load
template <>  FLASHMEM
bool updateFromControls<ContextWaveformModulated>(ContextWaveformModulated* myContext, AudioObjInstance* aoi)
{
  bool result = false;

  if (selectArbWAVfile(myContext, aoi))
  {
    result = true; // don't exit parent settings page
  }
  else
  {
    for (size_t i=0; i < myContext->paramCount; i++)
    {
      if (1 == i) // frequency
      {
        enc0.available();
        if (ScaleFreq(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
        {
          settingsEditor->ShowValue(i);
          myContext->setParam(i,aoi);
        }      
      }
      else
      {
        // Note this is safe for the arbitrary waveform, because
        // Scale() is always false for a string value
        if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
        {
          settingsEditor->ShowValue(i);
          myContext->setParam(i,aoi);
        }
      }
    }

    pollFileSelect(myContext, enc5);
  }
  
  return result;
}


template <> FLASHMEM
void processMIDIevent<ContextWaveformModulated>(AudioObjInstance* aoi, MIDIevent* ev)
{
  processMIDIforWaveform(aoi,ev,(ContextWaveformModulated*) aoi->context,aoi->streamP.WaveformModulated);
}
  
FLASHMEM int editWaveformModulated(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  // fix up arbitrary waveform if we're about to be destroyed
  if (nullptr != aoi->context)
    ((ContextWaveformModulated*) (aoi->context))->fixArbWAV(aoi->streamP.WaveformModulated, mode);

  int result = editObjType<AudioSynthWaveformModulated, ContextWaveformModulated>(aoi,mode,params);

  // Only after construction do we have a context with 
  // a default arbitrary waveform.
  if (nullptr != aoi->context)
    ((ContextWaveformModulated*) (aoi->context))->fixArbWAV(aoi->streamP.WaveformModulated, mode);
  
  return result;
}


//===========================================================================================
PROGMEM constexpr ParamEntry ContextKarplusStrong::MIDIparams[] 
{
  {"   octave", 0, 9}, // middle C = 261.63Hz = note#60 = octave 4
  {"   detune", -6.00f, +6.00f}, // semitones / cents
  {" velocity",PARAM_ENTRY_CHOICES(velocityShapes)},
  {"   tuning",PARAM_ENTRY_CHOICES(tuningTypes)},
  {"PB amount",0.0f, 12.0f},
};


PROGMEM constexpr ParamEntry ContextKarplusStrong::_params[5] = 
{
  {" frequency", -4.0f, 14.0f, 'l'}, // log2(freq) is what we actually store
  {" amplitude", 0.0f, 1.0f},
  {"modulation", 0.1f, 2.0f},
  {"  feedback", 0.9f, 1.0f},
  {"     drive", 0.0f, 1.0f},
};

FLASHMEM void ContextKarplusStrong::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    default: break;
    case 0: 
    case 1: /* aoi->streamP.KarplusStrong->noteOn(pow(2,s.frequency.value.f),s.amplitude.value.f); */ break;
    case 2: aoi->streamP.KarplusStrong->frequencyModulation(s.modulation.value.f); break;
    case 3: aoi->streamP.KarplusStrong->setFeedbackLevel(s.feedback.value.f); break;
    case 4: aoi->streamP.KarplusStrong->setDriveLevel(s.drive.value.f); break;
  }
}

template <> FLASHMEM
void enterEditMode<ContextKarplusStrong>(ContextKarplusStrong* myContext, AudioObjInstance* aoi)
{
  // fix up the pot and encoder values
  int iprt = floor(myContext->s.frequency.value.f - LOG_NOTE_A);
  float frac = myContext->s.frequency.value.f - iprt - LOG_NOTE_A;

  // Serial.printf("freq is %f -> %f Hz\n",myContext->s.frequency.value.f,pow(2,myContext->s.frequency.value.f));
  
  // octave encoder
  enc0.setLimits(-3,12);
  if (frac > 0.5f)
  {
    frac -= 1.0f;
    iprt++;
  }
  enc0.setValue(iprt);

  ParamValue pv{frac};    
  HookControl(ctrl,0,freqLimits,pv); // frequency pot is #0: set hook
}
  

template <> // template specialization for setting ContextKarplusStrong; needed for frequency setting
bool updateFromControls<ContextKarplusStrong>(ContextKarplusStrong* myContext, AudioObjInstance* aoi)
{
  for (size_t i=0; i < myContext->paramCount; i++)
  {
    if (0 == i) // frequency
    {
      enc0.available();
      if (ScaleFreq(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
      {
        settingsEditor->ShowValue(i);
        myContext->setParam(i,aoi);
      }      
    }
    else
    {
      if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
      {
        settingsEditor->ShowValue(i);
        myContext->setParam(i,aoi);
      }
    }
  }
  return false;
}


template <> FLASHMEM
void processMIDIevent<ContextKarplusStrong>(AudioObjInstance* aoi, MIDIevent* ev)
{
  processMIDIforKarplusStrong(aoi,ev,(ContextKarplusStrong*) aoi->context,aoi->streamP.KarplusStrong);
}

// \return 0 for idle, 1 for active
template <> FLASHMEM
int isActive<ContextKarplusStrong>(AudioObjInstance* aoi)
{
  return aoi->streamP.KarplusStrong->isPlaying()?1:0;
}
  
FLASHMEM int editKarplusStrong(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  int result = editObjType<AudioSynthKarplusStrongModulated, ContextKarplusStrong>(aoi,mode,params);
  return result;    
}

//===========================================================================================

//===========================================================================================
PROGMEM constexpr ParamEntry ContextWaveformDc::_params[] = {
  {"value", -1.0f, 1.0f},
};


PROGMEM constexpr ParamEntry ContextWaveformDc::MIDIparams[]
{                         // -4   -3   -2     -1        0           1           2    ...
  {"CC number", -4, 119}, // off, PB, note, velocity, bank select, modulation, breath...
  {"min", -1.00f, +1.00f},
  {"max", -1.00f, +1.00f},
};


void ContextWaveformDc::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.WaveformDc->amplitude(amplitude.value.f); break;
  }
}

FLASHMEM int editWaveformDc(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthWaveformDc, ContextWaveformDc>(aoi,mode,params);    
}

template <> FLASHMEM
void processMIDIevent<ContextWaveformDc>(AudioObjInstance* aoi, MIDIevent* ev)
{
  ContextWaveformDc* ctxt = (ContextWaveformDc*) aoi->context;
  
  switch (ev->type)
  {
    case midi::ControlChange:
    {
      if (ev->CCnum == ctxt->m.CCnum.value.i)
      {
        float ampl = map((float) ev->CCval,0.0f,127.0f,ctxt->m.CCmin.value.f,ctxt->m.CCmax.value.f);
        aoi->streamP.WaveformDc->amplitude(ampl);
      }
    }
      break;

    case midi::NoteOn:
    {
      int CCval = 0;
      PatcherVoice* pv = (PatcherVoice*) &(ev->pvb);
      
      switch (ctxt->m.CCnum.value.i)
      {
        case -4: // off
          CCval = -1;
          break;
          
        case -2: // set amplitude from note value
          CCval = pv->getNote();
          break;
          
        case -1: // set amplitude from note velocity
          CCval = pv->getVelocity();
          break;

        default: // normal CC            
          CCval = ev->pvb.pm.getCC(ctxt->m.CCnum.value.i);
          break;
      }

      if (CCval >= 0) // valid CC value
      {
        float ampl = map((float) CCval,0.0f,127.0f,ctxt->m.CCmin.value.f,ctxt->m.CCmax.value.f);
        aoi->streamP.WaveformDc->amplitude(ampl);          
      }
    }
      break;

    case midi::PitchBend:
      if (-3 == ctxt->m.CCnum.value.i)
      {
        int16_t PBval = ev->pvb.pm.getPitchBend(); // ±8191
        float ampl = map((float) PBval,-8191.0f,8191.0f,ctxt->m.CCmin.value.f,ctxt->m.CCmax.value.f);
        aoi->streamP.WaveformDc->amplitude(ampl);          
      }
      break;      
  }
}

//===========================================================================================
PROGMEM constexpr ParamEntry ContextNoise::_params[] = {
  {"amplitude", 0.0f, 1.0f}
};

FLASHMEM void ContextNoise::setParam(int i, AudioObjInstance* aoi)
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

FLASHMEM int editNoiseWhite(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthNoiseWhite, ContextNoise>(aoi,mode,params);    
}

FLASHMEM int editNoisePink(AudioObjInstance* aoi, AudioEditMode mode, void* params)
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

FLASHMEM void ContextControlSGTL5000::setParam(int i, AudioObjInstance* aoi)
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

template <> FLASHMEM
// Template specialization for creating a new 
//                                   VVVV
void editCreateStream<AudioControlSGTL5000>(AudioObjInstance* aoi, AudioObjInstance* original)
{ 
  aoi->streamP.ControlSGTL5000 = new AudioControlSGTL5000; 
  //Serial.printf("Constructed new %s for %08X at %08X\n", aoi->objP->name, (uint32_t) aoi, (uint32_t) aoi->streamP.streamObj);
}

FLASHMEM int editControlSGTL5000(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioControlSGTL5000, ContextControlSGTL5000>(aoi,mode,params);
}

//===========================================================================================
 const ParamChoice ladderInterpolation[] = 
  {  
    {"FIR poly", LADDER_FILTER_INTERPOLATION_FIR_POLY},
    {"linear", LADDER_FILTER_INTERPOLATION_LINEAR}
  };
  
FLASHMEM void ContextLadder::setParam(int i, AudioObjInstance* aoi)
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

PROGMEM constexpr ParamEntry ContextLadder::_params[6]  {
        {"    frequency", 3.0f, 13.2877123795495f, 'l'}, // 8.0 .. 10,000.0 Hz
        {"    resonance", 0.0f, 1.8f},
        {"      octaves", 0.0f, 7.0f},
        {"        drive", 0.0f, 4.0f},
        {"passband gain", 0.0f, 0.5f},
        {"interpolation", PARAM_ENTRY_CHOICES(ladderInterpolation)}
        //{"interpolation", ladderInterpolation, 2}
    };

FLASHMEM int editLadder(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioFilterLadder, ContextLadder>(aoi,mode,params);
}

//===========================================================================================
FLASHMEM int editMultiply(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectMultiply, ContextMultiply>(aoi,mode,params);    
}
//===========================================================================================
const ContextMIDInote filterNoteContext; // dummy context for filter tracking purposes
//===========================================================================================
FLASHMEM void ContextStateVariable::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.StateVariable->frequency(pow(2,s.frequency.value.f)); break;
    case 1: aoi->streamP.StateVariable->resonance(s.resonance.value.f); break;
    case 2: aoi->streamP.StateVariable->octaveControl(s.octaves.value.f); break;
  }
}

PROGMEM constexpr ParamEntry ContextStateVariable::_params[4] = {
        {"frequency", 3.0f, 13.2877123795495f, 'l'}, // 8.0 .. 10,000.0 Hz
        {"resonance", 0.7f, 5.0f},
        {"  octaves", 0.0f, 7.0f},
        {" tracking", 0.0f, 3.0f}
    };

FLASHMEM int editStateVariable(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioFilterStateVariable, ContextStateVariable>(aoi,mode,params);
}

PROGMEM constexpr ParamEntry ContextStateVariable::MIDIparams[]
{
  {"Resonance CC", -1, 119}, // off, bank select, modulation, breath...
  {"min", 0.7f, +5.00f},
  {"max", 0.7f, +5.00f},
};


template <> FLASHMEM
void processMIDIevent<ContextStateVariable>(AudioObjInstance* aoi, MIDIevent* ev)
{
  ContextStateVariable* ctxt = (ContextStateVariable*) aoi->context;
  
  switch (ev->type)
  {
    case midi::NoteOn:
      {
        float freq,ampl_unused;

        processMIDItoFreqAndAmp(&filterNoteContext,ev,freq,ampl_unused);
        // Magic calculation to set cutoff frequency from note.
        // We say the setting is the cutoff frequency that's correct for middle C,
        // and scale accordingly, but adjusted for tracking. Tracking=1.0
        // scales 1:1 with note frequency, 0.0 doesn't scale at all, and so on
        freq = pow(2,ctxt->s.frequency.value.f) 
                * (ctxt->s.tracking.value.f * (freq/MIDDLE_C - 1.0f) + 1.0f);
        if (freq < ctxt->MIN_CUTOFF) // lop off insane cutoff frequencies
          freq = ctxt->MIN_CUTOFF;
        aoi->streamP.StateVariable->frequency(freq);
      }
      break;

    case midi::ControlChange:
    {
      if (ev->CCnum == ctxt->m.CCnum.value.i)
      {
        float ampl = map((float) ev->CCval,0.0f,127.0f,ctxt->m.CCmin.value.f,ctxt->m.CCmax.value.f);
        aoi->streamP.StateVariable->resonance(ampl);
      }
    }
    break;
  }
}

//===========================================================================================
PROGMEM constexpr ParamEntry ContextWaveform::MIDIparams[] 
{
  {"   octave", 0, 9}, // middle C = 261.63Hz = note#60 = octave 4
  {"   detune", -6.00f, +6.00f}, // semitones / cents
  {" velocity",PARAM_ENTRY_CHOICES(velocityShapes)},
  {"   tuning",PARAM_ENTRY_CHOICES(tuningTypes)},
  {"PB amount",0.0f, 12.0f},
};

PROGMEM constexpr ParamEntry ContextWaveform::_params[] = {
  {"  waveform", PARAM_ENTRY_CHOICES(waveShapes)},
  {" frequency", -4.0f, 14.0f, 'l'}, // log2(freq) is what we actually store
  {" amplitude", 0.0f, 1.0f},
  {"pulseWidth", 0.0f, 1.0f},
  {"    offset", -1.0f, 1.0f},
  {"  arb. WAV", 'w'}
};


FLASHMEM void ContextWaveform::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.Waveform->begin(waveShapes[s.waveform.value.i].value); break;
    case 1: 
      noteFreq = pow(2,s.frequency.value.f);
      aoi->streamP.Waveform->frequency(noteFreq); break;
    case 2: aoi->streamP.Waveform->amplitude(s.amplitude.value.f); break;
    case 3: aoi->streamP.Waveform->pulseWidth(s.pulseWidth.value.f); break;
    case 4: aoi->streamP.Waveform->offset(s.offset.value.f); break;
    case 5:
    case ARBWAV_PARAM:
    {
      arbWAVrecord<int16_t>& arb = *s.arbWAV.value.w;

      if (arb.loadIfNeeded())
        aoi->streamP.Waveform->arbitraryWaveform(arb.sampleData,10000.0f);
    }
      break;
  }
}

template <> FLASHMEM
void enterEditMode<ContextWaveform>(ContextWaveform* myContext, AudioObjInstance* aoi)
{
  // fix up the pot and encoder values
  int iprt = floor(myContext->s.frequency.value.f - LOG_NOTE_A);
  float frac = myContext->s.frequency.value.f - iprt - LOG_NOTE_A;

  // Serial.printf("freq is %f -> %f Hz\n",myContext->s.frequency.value.f,pow(2,myContext->s.frequency.value.f));
  
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

  enc0.setLED(LimitedEncoder::colour(1));
  enc4.setLED(LimitedEncoder::colour(5));
}

template <> FLASHMEM
void exitEditMode<ContextWaveform>(ContextWaveform* myContext, AudioObjInstance* aoi)
{
  enc0.setLED(0);
  enc4.setLED(0);
}

template <> // template specialization for setting Waveform; needed for frequency setting
bool updateFromControls<ContextWaveform>(ContextWaveform* myContext, AudioObjInstance* aoi)
{
  bool result = false;

  if (selectArbWAVfile(myContext, aoi))
  {
    result = true; // don't exit parent settings page
  }
  else
  {
    for (size_t i=0; i < myContext->paramCount; i++)
    {
      if (1 == i) // frequency
      {
        enc0.available();
        if (ScaleFreq(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
        {
          settingsEditor->ShowValue(i);
          myContext->setParam(i,aoi);
        }      
      }
      else
      {
        if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
        {
          settingsEditor->ShowValue(i);
          myContext->setParam(i,aoi);
        }
      }
    }

    pollFileSelect(myContext, enc4);
  }
  return result;
}

template <> FLASHMEM
void processMIDIevent<ContextWaveform>(AudioObjInstance* aoi, MIDIevent* ev)
{
  processMIDIforWaveform(aoi,ev,(ContextWaveform*) aoi->context,aoi->streamP.Waveform);
}

  
FLASHMEM int editWaveform(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  // fix up arbitrary waveform if we're about to be destroyed
  if (nullptr != aoi->context)
    ((ContextWaveform*) (aoi->context))->fixArbWAV(aoi->streamP.Waveform, mode);

  int result = editObjType<AudioSynthWaveform, ContextWaveform>(aoi,mode,params);
  // Only after construction do we have a context with a default arbitrary 
  // waveform. Also, may need to free it on destruction
  if (nullptr != aoi->context)
    ((ContextWaveform*) (aoi->context))->fixArbWAV(aoi->streamP.Waveform, mode);

  return result;
}

//===========================================================================================
PROGMEM constexpr ParamEntry ContextWavetable::MIDIparams[] 
{
  {"   octave", 0, 9}, // middle C = 261.63Hz = note#60 = octave 4
  {"   detune", -6.00f, +6.00f}, // semitones / cents
  {" velocity",PARAM_ENTRY_CHOICES(velocityShapes)},
  {"   tuning",PARAM_ENTRY_CHOICES(tuningTypes)},
  {"PB amount",0.0f, 12.0f}
};

PROGMEM constexpr ParamEntry ContextWavetable::_params[3] = {
  {" file", 't'},
  {"entry", 0, 10000},
  {"amplitude", 0.0f, 1.0f}
};


FLASHMEM void ContextWavetable::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    default: break;

    case 0:
    case WTINST_PARAM:
    {
      arbWAVrecord<AudioSynthWavetable::instrument_data>& arb = *s.sf2file.value.t;
      //arbWAV.index = s.index.value.i;

      if (arb.loadIfNeeded())
        aoi->streamP.Wavetable->setInstrument(*arb.sampleData);
    }
      break;

    case 1: // index
      //arbWAV.index = s.index.value.i;
      break;

    case 2: aoi->streamP.Wavetable->amplitude(s.amplitude.value.f); break;
  }
}


template <> FLASHMEM // template specialization for setting Wavetable; needed for frequency setting
bool updateFromControls<ContextWavetable>(ContextWavetable* myContext, AudioObjInstance* aoi)
{
  bool result = false;

  do
  {
    if (selectArbWAVfile(myContext, aoi))
    {
      result = true; // don't exit parent settings page
      break;
    }

    if (selectInstrument(myContext, aoi))
    {
      result = true; // don't exit parent settings page
      break;
    }

    for (size_t i=0; i < myContext->paramCount; i++)
    {
      switch (i)
      { // no special action
        default:
          if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
          {
            settingsEditor->ShowValue(i);
            myContext->setParam(i,aoi);
          }
          break;

         case 1: // instrument index - disable pot
          break;          
      }
    }

    pollFileSelect(myContext, enc0);
    pollInstSelect(myContext, enc1);
  } while (0);
  
  return result;
}

template <> FLASHMEM
void processMIDIevent<ContextWavetable>(AudioObjInstance* aoi, MIDIevent* ev)
{
  processMIDIforWavetable(aoi,ev,(ContextWavetable*) aoi->context,aoi->streamP.Wavetable);
}

// \return 0 for idle, 1 for active
template <> FLASHMEM
int isActive<ContextWavetable>(AudioObjInstance* aoi)
{
  return aoi->streamP.Wavetable->isPlaying()?1:0;
}
  
FLASHMEM int editWavetable(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  int result = editObjType<AudioSynthWavetable, ContextWavetable>(aoi,mode,params);

  return result;
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
  
PROGMEM constexpr ParamEntry ContextBiquad::_params[] = {
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

FLASHMEM void ContextBiquad::setParam(int i, AudioObjInstance* aoi)
{
  int stge = s.stage.value.i;

  if (prevStage != stge // selected a different filter stage...
   && nullptr != settingsEditor)    // ...and we're editing
  {
    for (size_t i=0; i < paramCount; i++) // display the values
    {
      if (0 != i) // not the stage number!
        stageSettings[prevStage][i] = aray[i].value; // store edited values
      aray[i].value = stageSettings[stge][i];      // copy new values
      settingsEditor->ShowValue(i);  // display them
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

FLASHMEM int editBiquad(AudioObjInstance* aoi, AudioEditMode mode, void* params)
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
FLASHMEM void ContextEnvelope::setParam(int i, AudioObjInstance* aoi)
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

PROGMEM constexpr ParamEntry ContextEnvelope::_params[7] = {
        {"    delay", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"   attack", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"     hold", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"    decay", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"  sustain", 0.0f, 1.0f}, 
        {"  release", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"relNoteOn", 0.0f, 13.2877123795495f, 'l'} // 1.0 .. 10,000.0ms
    };

FLASHMEM int editEnvelope(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectEnvelope, ContextEnvelope>(aoi,mode,params);
}

template <> FLASHMEM
void processMIDIevent<ContextEnvelope>(AudioObjInstance* aoi, MIDIevent* ev)
{
  if (midi::NoteOff == ev->type) // note off
    aoi->streamP.Envelope->noteOff();
  if (midi::NoteOn  == ev->type) // note on
    aoi->streamP.Envelope->noteOn();
}

// \return 0 for idle, 2 for active, 3 for sustain; should never be 1
template <> FLASHMEM
int isActive<ContextEnvelope>(AudioObjInstance* aoi)
{
  return (aoi->streamP.Envelope->isSustain()?1:0)
       + (aoi->streamP.Envelope->isActive() ?2:0);
}

//===========================================================================================
FLASHMEM void ContextExpEnvelope::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: aoi->streamP.ExpEnvelope->delay(pow(2,s.delay.value.f)); break;
    case 1: aoi->streamP.ExpEnvelope->attack(pow(2,s.attack.value.f),s.shape.value.f); break;
    case 2: aoi->streamP.ExpEnvelope->hold(pow(2,s.hold.value.f)); break;
    case 3: aoi->streamP.ExpEnvelope->decay(pow(2,s.decay.value.f),s.shape.value.f); break;
    case 4: aoi->streamP.ExpEnvelope->sustain(s.sustain.value.f); break;
    case 5: aoi->streamP.ExpEnvelope->release(pow(2,s.release.value.f),s.shape.value.f); break;
    case 6:
      setParam(1,aoi); 
      setParam(3,aoi); 
      setParam(5,aoi); 
      break;
  }
}

PROGMEM constexpr ParamEntry ContextExpEnvelope::_params[7] = {
        {"  delay", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {" attack", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"   hold", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"  decay", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"sustain", 0.0f, 1.0f}, 
        {"release", 0.0f, 13.2877123795495f, 'l'}, // 1.0 .. 10,000.0ms
        {"  shape", 0.1f, 0.99f} // 
    };

FLASHMEM int editExpEnvelope(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectExpEnvelope, ContextExpEnvelope>(aoi,mode,params);
}

template <> FLASHMEM
void processMIDIevent<ContextExpEnvelope>(AudioObjInstance* aoi, MIDIevent* ev)
{
  if (midi::NoteOff == ev->type) // note off
    aoi->streamP.ExpEnvelope->noteOff();
  if (midi::NoteOn  == ev->type) // note on
    aoi->streamP.ExpEnvelope->noteOn();
}

// \return 0 for idle, 2 for active, 3 for sustain, should never be 3
template <> FLASHMEM
int isActive<ContextExpEnvelope>(AudioObjInstance* aoi)
{
  return (aoi->streamP.ExpEnvelope->isSustain()?1:0)
       + (aoi->streamP.ExpEnvelope->isActive() ?2:0);
}

//===========================================================================================
 const ParamChoice modesHammondVibrato[] = 
  {{"(off)",   0},
   {"vibrato", 1},
   {"chorus",  2},
  };

FLASHMEM void ContextHammondVibrato::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    case 0: 
      switch (modesHammondVibrato[s.mode.value.i].value)
      {
        case 0:
          aoi->streamP.HammondVibrato->enable(false); 
          break;

        case 1:
          aoi->streamP.HammondVibrato->enable(true); 
          aoi->streamP.HammondVibrato->vibrato_mode(); 
          break;

        case 2:
          aoi->streamP.HammondVibrato->enable(true); 
          aoi->streamP.HammondVibrato->chorus_mode(); 
          break;

      }
      break;
      
    case 1: 
      aoi->streamP.HammondVibrato->set_depth(s.depth.value.i); 
      break;
  }
}

PROGMEM constexpr ParamEntry ContextHammondVibrato::_params[2] = {
        {" mode", PARAM_ENTRY_CHOICES(modesHammondVibrato)},
        {"depth", 1, 3 }, 
    };

FLASHMEM int editHammondVibrato(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioEffectHammondVibrato, ContextHammondVibrato>(aoi,mode,params);
}


//===========================================================================================
FLASHMEM void ContextDexed::setParam(int i, AudioObjInstance* aoi)
{
  switch (i)
  {
    default: break;
    case 0: aoi->streamP.Dexed->setGain(s.gain.value.f); break;
    case 1: aoi->streamP.Dexed->setPitchbendRange(s.bend.value.i); break;
  }
}

PROGMEM constexpr ParamEntry ContextDexed::_params[2] = 
{
  {"gain", 0.0f, 1.0f},
  {"bend", 0, 12}
};

extern uint8_t fmpiano_sysex[];
template <> FLASHMEM
// Template specialization for creating a new 
//                              VVVV
void editCreateStream<AudioSynthDexed>(AudioObjInstance* aoi, AudioObjInstance* original)
{ 
  if (!aoi->isAcopy)
  {
    aoi->streamP.Dexed = new AudioSynthDexed{AudioSynthDexed_CONSTRUCTOR_1};
    aoi->streamP.Dexed->loadVoiceParameters(&fmpiano_sysex[0]);
    aoi->streamP.Dexed->setPitchbendRange(3); // default is no pitch bend, for some reason
  }
  else
    aoi->streamP.Dexed = new AudioSynthDexed{AudioSynthDexed_CONSTRUCTOR_2};

}

FLASHMEM int editDexed(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  return editObjType<AudioSynthDexed, ContextDexed>(aoi,mode,params);    
}

template <> FLASHMEM
void processMIDIevent<ContextDexed>(AudioObjInstance* aoi, MIDIevent* ev)
{
  switch (ev->type)
  {
    case midi::NoteOn: // note on
      aoi->streamP.Dexed->keydown(ev->note, ev->velocity);
      break;
  
    case midi::NoteOff: // note off
      aoi->streamP.Dexed->keyup(ev->note);
      break;

    case midi::PitchBend: // pitchbend
      aoi->streamP.Dexed->setPitchbend(ev->PBlsb, ev->PBmsb);
      break;

    case midi::SystemExclusive:
    {
/*
Serial.printf("SysEx @ %08X: length = %d; data %02X %02X %02X %02X %02X %02X %02X ...", 
  (uint32_t) ev->sysexArray,
  ev->sysexLength,
  ev->sysexArray[0],
  ev->sysexArray[1],
  ev->sysexArray[2],
  ev->sysexArray[3],
  ev->sysexArray[4],
  ev->sysexArray[5],
  ev->sysexArray[6]
);
//*/
      switch (ev->sysexLength)
      {
        default:
          break; 

        case 163:
        {
          uint8_t hdr[] = {0xF0, 0x43, (uint8_t) (ev->sysexArray[2] & 0x0F), 0x00, 0x01, 0x1B};
          bool hdrOK = 0 == memcmp(ev->sysexArray,hdr,sizeof hdr);
          uint8_t sum = 0;
          for (int i=0;i<155;i++)
            sum -= ev->sysexArray[i+6];
          sum &= 0x7F; // sysex can only have 0-127 values
/*
Serial.printf("; sum %02X; check %02X; header %sOK", 
    sum,
    ev->sysexArray[161],
    hdrOK?"":"not "
  );
*/

          if (hdrOK && sum == ev->sysexArray[161])
          {
//            char buf[11];
            aoi->streamP.Dexed->loadVoiceParameters(ev->sysexArray + 6);
//            aoi->streamP.Dexed->getName(buf);
// Serial.printf("; loaded %s", buf);
          }
        }
          break;

        case 7:
        {
          uint8_t hdr[] = {0xF0, 0x43, 
                           (uint8_t) ((ev->sysexArray[2] & 0x0F) + 0x10),
                           (uint8_t)  (ev->sysexArray[3] & 0x09)};
          bool hdrOK = 0 == memcmp(ev->sysexArray,hdr,sizeof hdr);
          if (hdrOK)
          {
            int idx = ((ev->sysexArray[3] & 0x03)?128:0) + ev->sysexArray[4];
            aoi->streamP.Dexed->setVoiceDataElement(idx,ev->sysexArray[5]);
//Serial.printf("element %d -> %d",idx,ev->sysexArray[5]);
            aoi->streamP.Dexed->doRefreshVoice();
            // for some reason Dexed doesn't honour the operator on/off switches
            if (155 == idx) 
              aoi->streamP.Dexed->setOPAll(ev->sysexArray[5]);
          }
        }
          break;
      }
//Serial.println();
      break;
    }
      break;
  }
}

// \return 1 if active, 0 otherwise
template <> FLASHMEM
int isActive<ContextDexed>(AudioObjInstance* aoi)
{
  int playCount = aoi->streamP.Dexed->getNumNotesPlaying();
  return playCount>0;
}