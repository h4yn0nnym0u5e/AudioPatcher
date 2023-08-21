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
  int modeI = (int) mode;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  if (0 == modeI || -1 == modeI)
  {
    Serial.printf("It's a %s object! %s! At %08X; systemState = %d @ %08X\n",
                  aoi->objP->name,
                  modeI==-1?"Destroyed":"Constructed",
                  (uint32_t) aoi,
                  systemState,(uint32_t) &display
                  ); 
  }
  
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
  int modeI = (int) mode;
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
  {"frequency", 10.0f, 10000.0f},
  {"amplitude", 0.0f, 1.0f},
  {"offset", -1.0f, 1.0f},
  {"mod type",PARAM_ENTRY_CHOICES(modTypes)},
  {"mod depth",0.0f, 12.0f} // frequency is 0.0 - 12.0 octaves; phase modulation could be 9000°
};
struct contextWaveformModulated {
  union { struct {ParamValue waveform,frequency,amplitude,offset,modType,modDepth;} s;
          ParamValue aray[6];
        };
  };
int editWaveformModulated(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextWaveformModulated* myContext = (contextWaveformModulated*) aoi->context;
  AudioSynthWaveformModulated* me = aoi->streamP.WaveformModulated;
  int modeI = (int) mode;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new contextWaveformModulated{{{{0},{220.0f},{0.0f},{0.0f},{0},{1.0f}}}};
        aoi->context = myContext;
        //for (size_t i=0;i<COUNT_OF(myContext->gain);i++)
        me->begin(myContext->s.waveform.value.i);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      delete myContext;
      break;

    case AudioEditMode::enter:
      result = 1; // claimed
      next = millis();
      pe = new ParamEditor(display,BOX_DEF(250,COUNT_OF(paramsWaveformModulated)));
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0;i<COUNT_OF(paramsWaveformModulated);i++)
      {
        pe->display.ShowLabel(paramsWaveformModulated[i],myContext->aray[i],i,5,27);
        pe->display.ShowValue(paramsWaveformModulated[i],myContext->aray[i],i);
        HookControl(ctrl,i,paramsWaveformModulated[i],myContext->aray[i]);
      }
      break;      

    case AudioEditMode::edit:
      if (millis() > next)
      {
        next = millis() + 10;
        
        for (size_t i=0;i<COUNT_OF(paramsWaveformModulated);i++)
        {
          if (Scale(paramsWaveformModulated[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
          {
            pe->display.ShowValue(paramsWaveformModulated[i],myContext->aray[i],i); 
            switch (i)
            {
              case 0: aoi->streamP.WaveformModulated->begin(myContext->s.waveform.value.i); break;
              case 1: aoi->streamP.WaveformModulated->frequency(myContext->s.frequency.value.f); break;
              case 2: aoi->streamP.WaveformModulated->amplitude(myContext->s.amplitude.value.f); break;
              case 3: aoi->streamP.WaveformModulated->offset(myContext->s.offset.value.f); break;

              // these need special treatment:
              //case 4: aoi->streamP.WaveformModulated->begin(myContext->s.waveform.value.i); break;
              //case 5: aoi->streamP.WaveformModulated->begin(myContext->s.waveform.value.i); break;
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
