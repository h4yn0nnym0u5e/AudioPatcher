#include "objects.h"
#include "display.h"
#include "paramsetters.h"
#include "limitedEncoder.h"

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)

extern LimitedEncoder encM,enc0,enc1,enc2;
extern M5w_8angle ctrl;
#define M5ANGLE_MIN   20
#define M5ANGLE_MAX 4080

#define BOX_DEF(width,lines) (320/2 - (width)/2),(240/2 - (27+(lines)*16+16)/2),(width),(27+(lines)*16+16)

//===========================================================================================
ParamEditor* pe;

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
  newVal = newVal + (float) pow2 + 0.781359714f;
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
  bool result;
  if (1 == pe.type)  
    result = ScaleF(pe,pv,raw, filter);
  else
    result = ScaleI(pe,pv,raw);

  return result;
}

void HookControl(M5w_8angle& ctrl, int ch, const ParamEntry& pe, ParamValue& pv)
{
  if (1 == pe.type)  // float
    ctrl.setHook(ch,map(pv.value.f,pe.min.f,pe.max.f,M5ANGLE_MIN,M5ANGLE_MAX));
  else
    ctrl.setHook(ch,map(pv.value.i,pe.min.i,pe.max.i,M5ANGLE_MIN,M5ANGLE_MAX));
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
// Strong definitions of setup controls
const ParamEntry paramsChorus[] = {
  {"length [ms]", 10.0f, 200.0f},
  {"voices", 1, 20},
};
struct contextChorus {union {struct {ParamValue length,voices;} s; ParamValue aray[2];}; short* mem; };
int editChorus(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextChorus* myContext = (contextChorus*) aoi->context;
  AudioEffectChorus* me = aoi->streamP.Chorus;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {
        
        myContext = new contextChorus{{{{25.0f},{1}}},nullptr};
        aoi->context = myContext;
        size_t memsize = myContext->s.length.value.f / 1000.0f * AUDIO_SAMPLE_RATE * sizeof(short); // the man say 50ms - do it
        myContext->mem = (short*) malloc(memsize);
        me->begin(myContext->mem,memsize,myContext->s.voices.value.i);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      free(myContext->mem);
      delete myContext;
      break;

    case AudioEditMode::enter:
      exitAt = 0;
      result = 1; // claimed
      next = millis();
      pe = new ParamEditor(display,BOX_DEF(260,2));
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0;i<COUNT_OF(paramsChorus);i++)
      {
        pe->display.ShowLabel(paramsChorus[i],myContext->aray[i],i,5,27);
        pe->display.ShowValue(paramsChorus[i],myContext->aray[i],i);
      }
      HookControl(ctrl,0,paramsChorus[0],myContext->s.length);
      HookControl(ctrl,1,paramsChorus[1],myContext->s.voices);
      break;      

    case AudioEditMode::edit:
      {
        if (millis() > next)
        {
          next = millis() + 10;
        
          if (Scale(paramsChorus[0],myContext->s.length,ctrl.getPot16(0),0.9f))
            pe->display.ShowValue(paramsChorus[0],myContext->s.length,0);  
          
          if (Scale(paramsChorus[1],myContext->s.voices,ctrl.getPot16(1)))
            pe->display.ShowValue(paramsChorus[1],myContext->s.voices,1);  
        }

        result = testExit(exitAt);
        if (0 == result)
        {
          me->begin(nullptr,0,0); // stop the chorus
          free(myContext->mem);
          
          size_t memsize = myContext->s.length.value.f / 1000.0f * AUDIO_SAMPLE_RATE * sizeof(short); // the man say 50ms - do it
          myContext->mem = (short*) malloc(memsize);
          memset(myContext->mem,0,memsize); // prevent burst of noise
          me->begin(myContext->mem,memsize,myContext->s.voices.value.i);
        }        
      }
      break;      

    case AudioEditMode::exit:
      delete pe;
      pe = nullptr;
      break;      

    default:
      break;      
  }
  return result;  
}

//===========================================================================================
const ParamEntry paramsMixer4[] = {
  {"ch1", 0.0f, 1.0f},
  {"ch2", 0.0f, 1.0f},
  {"ch3", 0.0f, 1.0f},
  {"ch4", 0.0f, 1.0f},
};
struct contextMixer4 {ParamValue gain[4];};
int editMixer4(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextMixer4* myContext = (contextMixer4*) aoi->context;
  AudioMixer4* me = aoi->streamP.Mixer4;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new contextMixer4{{0.5f,0.5f,0.5f,0.5f}};
        aoi->context = myContext;
        for (size_t i=0;i<COUNT_OF(myContext->gain);i++)
          me->gain(i,myContext->gain[i].value.f);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      delete myContext;
      break;

    case AudioEditMode::enter:
      result = 1; // claimed
      next = millis();
      pe = new ParamEditor(display,BOX_DEF(120,4));
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0;i<COUNT_OF(paramsMixer4);i++)
      {
        pe->display.ShowLabel(paramsMixer4[i],myContext->gain[i],i,5,27);
        pe->display.ShowValue(paramsMixer4[i],myContext->gain[i],i);
        HookControl(ctrl,i,paramsMixer4[i],myContext->gain[i]);
      }
      break;      

    case AudioEditMode::edit:
      if (millis() > next)
      {
        next = millis() + 10;
      
        for (size_t i=0;i<COUNT_OF(paramsMixer4);i++)
        {
          if (Scale(paramsMixer4[i],myContext->gain[i],ctrl.getPot16(i),0.999f))
          {
            pe->display.ShowValue(paramsMixer4[0],myContext->gain[i],i); 
            aoi->streamP.Mixer4->gain(i,myContext->gain[i].value.f);
          }
        }           
      }
      
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit:
      delete pe;
      pe = nullptr;
      break;      

    default:
      break;      
  }
  return result;    
}

//===========================================================================================
ParamChoice waveShapes[] = 
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

const ParamEntry paramsWaveformModulated[] = {
  {"waveform", PARAM_ENTRY_CHOICES(waveShapes)},
  {"frequency", -4.0f, 14.0f}, // log2(freq) is what we actually store
  {"amplitude", 0.0f, 1.0f},
  {"offset", -1.0f, 1.0f},
  {"mod type",PARAM_ENTRY_CHOICES(modTypes)},
  {"mod depth",0.0f, 12.0f} // frequency is 0.0 - 12.0 octaves; phase modulation could be 9000°
};
const ParamEntry freqLimits{nullptr,-1.0f,1.0f}; // special, for setting hook control
struct contextWaveformModulated {
  union { struct {ParamValue waveform,frequency,amplitude,offset,modType,modDepth;} s;
          ParamValue aray[COUNT_OF(paramsWaveformModulated)];
        };
  };
int editWaveformModulated(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextWaveformModulated* myContext = (contextWaveformModulated*) aoi->context;
  AudioSynthWaveformModulated* me = aoi->streamP.WaveformModulated;
  static uint32_t exitAt;
  static uint32_t next;
  static int32_t v,l,u; // storage for encoder state
  int result = 0;
  const int FREQ = 1; // needed to index stuff for special cases

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new contextWaveformModulated{{{{0},{7.0f},{0.5f},{0.0f},{0},{1.0f}}}};
        aoi->context = myContext;
        //for (size_t i=0;i<COUNT_OF(myContext->gain);i++)
        me->begin(myContext->s.amplitude.value.f,
                  pow(2,myContext->s.frequency.value.f),
                  waveShapes[myContext->s.waveform.value.i].value);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      delete myContext;
      break;

    case AudioEditMode::enter:
      {
        result = 1; // claimed
        next = millis();
        pe = new ParamEditor(display,BOX_DEF(250,COUNT_OF(paramsWaveformModulated)));
        pe->display.ShowTitle(aoi->objP->name,5,5);
  
        // change frequency from log2 to Hz
        float tmp = myContext->s.frequency.value.f;
        myContext->s.frequency.value.f = pow(2,tmp);
        
        for (size_t i=0;i<COUNT_OF(paramsWaveformModulated);i++)
        {          
          pe->display.ShowLabel(paramsWaveformModulated[i],myContext->aray[i],i,5,27);
          pe->display.ShowValue(paramsWaveformModulated[i],myContext->aray[i],i);
          HookControl(ctrl,i,paramsWaveformModulated[i],myContext->aray[i]);
        }

        // fix up the pot and encoder values
        int iprt = floor(tmp);
        float frac = tmp - iprt;
        
        enc0.available(); // force an update
        v = enc0.getValue();
        enc0.getLimits(l,u);
        enc0.setLimits(-3,12);
        enc0.setValue(frac > 0.5f?(iprt+1):iprt);
        if (frac > 0.5f)
          frac -= 1.0f;
        myContext->s.frequency.value.f = frac;
        HookControl(ctrl,FREQ,freqLimits,myContext->s.frequency);
        myContext->s.frequency.value.f = tmp; // back to log2 mode
      }
      break;      

    case AudioEditMode::edit:
      if (millis() > next)
      {
        next = millis() + 10;
        
        for (size_t i=0;i<COUNT_OF(paramsWaveformModulated);i++)
        {
          if (1 == i) // frequency - special case
          {
            enc0.available(); // force an update
            if (ScaleFreq(paramsWaveformModulated[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
            {
              float tmp = myContext->s.frequency.value.f; // keep value
              myContext->s.frequency.value.f = pow(2,tmp); // convert to true frequency
            
              pe->display.ShowValue(paramsWaveformModulated[i],myContext->aray[i],i); 
              aoi->streamP.WaveformModulated->frequency(myContext->s.frequency.value.f);
              myContext->s.frequency.value.f = tmp;
            }
          }
          else
          {
            if (Scale(paramsWaveformModulated[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
            {
              pe->display.ShowValue(paramsWaveformModulated[i],myContext->aray[i],i); 
              switch (i)
              {
                case 0: aoi->streamP.WaveformModulated->begin(waveShapes[myContext->s.waveform.value.i].value); break;
                case 2: aoi->streamP.WaveformModulated->amplitude(myContext->s.amplitude.value.f); break;
                case 3: aoi->streamP.WaveformModulated->offset(myContext->s.offset.value.f); break;
  
                // these need special treatment:
                //case 4: aoi->streamP.WaveformModulated->begin(myContext->s.waveform.value.i); break;
                //case 5: aoi->streamP.WaveformModulated->begin(myContext->s.waveform.value.i); break;
              }
            }
          }
        }                 
      }
      
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit:
      // restore encoder state
      enc0.setLimits(l,u);
      enc0.setValue(v);
      delete pe;
      pe = nullptr;
      break;      

    default:
      break;      
  }
  return result;    
}

//===========================================================================================
const ParamEntry paramsWaveform[] = {
  {"waveform", PARAM_ENTRY_CHOICES(waveShapes)},
  {"frequency", -4.0f, 14.0f}, // log2(freq) is what we actually store
  {"amplitude", 0.0f, 1.0f},
  {"pulseWidth", 0.0f, 1.0f},
  {"offset", -1.0f, 1.0f},
};
struct contextWaveform {
  union { struct {ParamValue waveform,frequency,amplitude,pulseWidth,offset;} s;
          ParamValue aray[COUNT_OF(paramsWaveform)];
        };
  };
int editWaveform(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextWaveform* myContext = (contextWaveform*) aoi->context;
  AudioSynthWaveform* me = aoi->streamP.Waveform;
  static uint32_t exitAt;
  static uint32_t next;
  static int32_t v,l,u; // storage for encoder state
  int result = 0;
  const int FREQ = 1; // needed to index stuff for special cases

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new contextWaveform{{{{0},{7.0f},{0.5f},{0.5f},{0.0f}}}};
        aoi->context = myContext;
        //for (size_t i=0;i<COUNT_OF(myContext->gain);i++)
        me->begin(myContext->s.amplitude.value.f,
                  pow(2,myContext->s.frequency.value.f),
                  waveShapes[myContext->s.waveform.value.i].value);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      delete myContext;
      break;

    case AudioEditMode::enter:
      {
        result = 1; // claimed
        next = millis();
        pe = new ParamEditor(display,BOX_DEF(250,COUNT_OF(paramsWaveform)));
        pe->display.ShowTitle(aoi->objP->name,5,5);
  
        // change frequency from log2 to Hz
        float tmp = myContext->s.frequency.value.f;
        myContext->s.frequency.value.f = pow(2,tmp);
        
        for (size_t i=0;i<COUNT_OF(paramsWaveform);i++)
        {          
          pe->display.ShowLabel(paramsWaveform[i],myContext->aray[i],i,5,27);
          pe->display.ShowValue(paramsWaveform[i],myContext->aray[i],i);
          HookControl(ctrl,i,paramsWaveform[i],myContext->aray[i]);
        }

        // fix up the pot and encoder values
        int iprt = floor(tmp);
        float frac = tmp - iprt;
        
        enc0.available(); // force an update
        v = enc0.getValue();
        enc0.getLimits(l,u);
        enc0.setLimits(-3,12);
        enc0.setValue(frac > 0.5f?(iprt+1):iprt);
        if (frac > 0.5f)
          frac -= 1.0f;
        myContext->s.frequency.value.f = frac;
        HookControl(ctrl,FREQ,freqLimits,myContext->s.frequency);
        myContext->s.frequency.value.f = tmp; // back to log2 mode
      }
      break;      

    case AudioEditMode::edit:
      if (millis() > next)
      {
        next = millis() + 10;
        
        for (size_t i=0;i<COUNT_OF(paramsWaveform);i++)
        {
          if (1 == i) // frequency - special case
          {
            enc0.available(); // force an update
            if (ScaleFreq(paramsWaveform[i],myContext->aray[i],ctrl.getPot16(i),enc0.getValue(),0.999f))
            {
              float tmp = myContext->s.frequency.value.f; // keep value
              myContext->s.frequency.value.f = pow(2,tmp); // convert to true frequency
            
              pe->display.ShowValue(paramsWaveform[i],myContext->aray[i],i); 
              aoi->streamP.Waveform->frequency(myContext->s.frequency.value.f);
              myContext->s.frequency.value.f = tmp;
            }
          }
          else
          {
            if (Scale(paramsWaveform[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
            {
              pe->display.ShowValue(paramsWaveform[i],myContext->aray[i],i); 
              switch (i)
              {
                case 0: aoi->streamP.Waveform->begin(waveShapes[myContext->s.waveform.value.i].value); break;
                case 2: aoi->streamP.Waveform->amplitude(myContext->s.amplitude.value.f); break;
                case 3: aoi->streamP.Waveform->pulseWidth(myContext->s.pulseWidth.value.f); break;
                case 4: aoi->streamP.Waveform->offset(myContext->s.offset.value.f); break;
              }
            }
          }
        }                 
      }
      
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit:
      // restore encoder state
      enc0.setLimits(l,u);
      enc0.setValue(v);
      delete pe;
      pe = nullptr;
      break;      

    default:
      break;      
  }
  return result;    
}

//===========================================================================================
const ParamEntry paramsWaveformDc[] = {
  {"value", -1.0f, 1.0f},
};
struct contextWaveformDc {ParamValue amplitude;};
int editWaveformDc(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextWaveformDc* myContext = (contextWaveformDc*) aoi->context;
  AudioSynthWaveformDc* me = aoi->streamP.WaveformDc;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new contextWaveformDc{{0.0f}};
        aoi->context = myContext;
        me->amplitude(myContext->amplitude.value.f);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      delete myContext;
      break;

    case AudioEditMode::enter:
      result = 1; // claimed
      next = millis();
      pe = new ParamEditor(display,BOX_DEF(160,1));
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0;i<COUNT_OF(paramsWaveformDc);i++)
      {
        pe->display.ShowLabel(paramsWaveformDc[i],myContext->amplitude,i,5,27);
        pe->display.ShowValue(paramsWaveformDc[i],myContext->amplitude,i);
        HookControl(ctrl,i,paramsWaveformDc[i],myContext->amplitude);
      }
      break;      

    case AudioEditMode::edit:
      if (millis() > next)
      {
        next = millis() + 10;
      
        for (size_t i=0;i<COUNT_OF(paramsWaveformDc);i++)
        {
          if (Scale(paramsWaveformDc[i],myContext->amplitude,ctrl.getPot16(i),0.999f))
          {
            pe->display.ShowValue(paramsWaveformDc[0],myContext->amplitude,i); 
            aoi->streamP.WaveformDc->amplitude(myContext->amplitude.value.f);
          }
        }           
      }
      
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit:
      delete pe;
      pe = nullptr;
      break;      

    default:
      break;      
  }
  return result;    
}

//===========================================================================================
ParamChoice inputsSGTL5000[] = 
  {{"line", AUDIO_INPUT_LINEIN},
   {"mic" , AUDIO_INPUT_MIC},
  };
  
ParamChoice avcSGTL5000[] = 
  {{"off", 0},
   {"on" , 1},
  };
  
ParamChoice inputLevelsSGTL5000[] = 
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

ParamChoice outputLevelsSGTL5000[] = 
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
  
const ParamEntry paramsControlSGTL5000[] = {
  {"        volume", 0.0f, 1.0f},
  {"  input select", PARAM_ENTRY_CHOICES(inputsSGTL5000)},
  {"  micGain [dB]", 0, 63},
  {"   auto volume", PARAM_ENTRY_CHOICES(avcSGTL5000)},
  {" input [Vpkpk]", PARAM_ENTRY_CHOICES(inputLevelsSGTL5000)},
  {"output [Vpkpk]", PARAM_ENTRY_CHOICES(outputLevelsSGTL5000)},
};
struct contextControlSGTL5000 {
  union { struct {ParamValue volume,inputSelect,micGain,autoVolume,lineInLevel,lineOutLevel;} s;
          ParamValue aray[COUNT_OF(paramsControlSGTL5000)];};};
int editControlSGTL5000(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextControlSGTL5000* myContext = (contextControlSGTL5000*) aoi->context;
  AudioControlSGTL5000* me = aoi->streamP.ControlSGTL5000;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new contextControlSGTL5000{{{ {0.25f},  {0},{20},{0}, {10},{11} }}};
        aoi->context = myContext;

        // find SGTL5000 at either address
        me->enable();
        if (!me->volume(myContext->s.volume.value.f))
        {
          me->setAddress(HIGH);
          me->enable();
          me->volume(myContext->s.volume.value.f);
        }
        
        me->micGain(myContext->s.micGain.value.i);
      }
      break;
      
    case AudioEditMode::destructor: // destruction (never happens!)
      delete myContext;
      break;

    case AudioEditMode::enter:
      result = 1; // claimed
      next = millis();
      pe = new ParamEditor(display,BOX_DEF(260,COUNT_OF(paramsControlSGTL5000)));
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0;i<COUNT_OF(paramsControlSGTL5000);i++)
      {
        pe->display.ShowLabel(paramsControlSGTL5000[i],myContext->aray[i],i,5,27);
        pe->display.ShowValue(paramsControlSGTL5000[i],myContext->aray[i],i);
        HookControl(ctrl,i,paramsControlSGTL5000[i],myContext->aray[i]);
      }
      break;      

    case AudioEditMode::edit:
      if (millis() > next)
      {
        next = millis() + 10;
      
        for (size_t i=0;i<COUNT_OF(paramsControlSGTL5000);i++)
        {
          if (Scale(paramsControlSGTL5000[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
          {
            pe->display.ShowValue(paramsControlSGTL5000[i],myContext->aray[i],i);

            switch (i)
            { // volume,inputSelect,micGain,lineInLevel,lineOutLevel,autoVolume
              case 0: aoi->streamP.ControlSGTL5000->volume(myContext->s.volume.value.f); break;
              case 1: aoi->streamP.ControlSGTL5000->inputSelect(inputsSGTL5000[myContext->s.inputSelect.value.i].value); break;
              case 2: aoi->streamP.ControlSGTL5000->micGain(myContext->s.micGain.value.i); break;
              
              case 4: aoi->streamP.ControlSGTL5000->lineInLevel(inputLevelsSGTL5000[myContext->s.lineInLevel.value.i].value); break;
              case 5: aoi->streamP.ControlSGTL5000->lineOutLevel(outputLevelsSGTL5000[myContext->s.lineOutLevel.value.i].value); break;
              
              case 3: 
                if (myContext->s.autoVolume.value.i)
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
        }           
      }
      
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit:
      delete pe;
      pe = nullptr;
      break;      

    default:
      break;      
  }
  return result;    
}
