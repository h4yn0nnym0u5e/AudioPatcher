#include "objects.h"
#include "display.h"
#include "paramsetters.h"
#include "limitedEncoder.h"

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)

extern LimitedEncoder encM,enc0,enc1,enc2;
extern M5w_8angle ctrl;
#define M5ANGLE_MIN   200
#define M5ANGLE_MAX 3050

//===========================================================================================
ParamEditor* pe;

//===========================================================================================
bool ScaleF(ParamEntry& pe, ParamValue& pv, int16_t raw, float filter)
{
  bool result = false;
  float newVal = constrain(map((float) raw, M5ANGLE_MIN, M5ANGLE_MAX, pe.min.f, pe.max.f), pe.min.f, pe.max.f);
  newVal = newVal*filter + pv.value.f*(1.0f - filter);
  if (fabs(newVal - pv.value.f) > 0.05)
  {
    pv.value.f = newVal;
    result = true;
  }
  return result;
}


bool ScaleI(ParamEntry& pe, ParamValue& pv, int16_t raw)
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

bool Scale(ParamEntry& pe, ParamValue& pv, int16_t raw, float filter = 1.0f)
{
  bool result;
  if (0 == pe.type)  
    result = ScaleI(pe,pv,raw);
  else
    result = ScaleF(pe,pv,raw, filter);

  return result;
}

void HookControl(M5w_8angle& ctrl, int ch, ParamEntry& pe, ParamValue& pv)
{
  if (0 == pe.type)  // integer
    ctrl.setHook(ch,map(pv.value.i,pe.min.i,pe.max.i,M5ANGLE_MIN,M5ANGLE_MAX));
  else
    ctrl.setHook(ch,map(pv.value.f,pe.min.f,pe.max.f,M5ANGLE_MIN,M5ANGLE_MAX));
}


//===========================================================================================
// Strong definitions of setup controls
ParamEntry paramsChorus[] = {
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
      pe = new ParamEditor(display,30,80,260,80);
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
        result = 1;

        if (millis() > next)
        {
          next = millis() + 10;
        
          if (Scale(paramsChorus[0],myContext->s.length,ctrl.getPot16(0),0.1f))
            pe->display.ShowValue(paramsChorus[0],myContext->s.length,0);  
          
          if (Scale(paramsChorus[1],myContext->s.voices,ctrl.getPot16(1)))
            pe->display.ShowValue(paramsChorus[1],myContext->s.voices,1);  
        }
        
        if (encM.getButton())
          exitAt = 1;
        else
        {
          if (exitAt)
          {
            me->begin(nullptr,0,0); // stop the chorus
            free(myContext->mem);
            
            size_t memsize = myContext->s.length.value.f / 1000.0f * AUDIO_SAMPLE_RATE * sizeof(short); // the man say 50ms - do it
            myContext->mem = (short*) malloc(memsize);
            memset(myContext->mem,0,memsize); // prevent burst of noise
            me->begin(myContext->mem,memsize,myContext->s.voices.value.i);
            
            exitAt = 0;
            result = 0;
          }
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
/*
ParamEntry paramMixer4[] = {
  {"ch1",0.05f, 0.0f, 1.0f},
  {"ch2",0.05f, 0.0f, 1.0f},
  {"ch3",0.05f, 0.0f, 1.0f},
  {"ch4",0.05f, 0.0f, 1.0f},
};
struct contextMixer4 {float gain[4];};
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
        
        myContext = new contextMixer4;
        for (size_t i=0;i<COUNT_OF(myContext->gain);i++)
        {
          me->gain(i,myContext->gain[i]);
        }
        myContext->delayTime = paramsMixer4[0].value.f; // milliseconds, for consistency
        size_t memsize = myContext->delayTime / 1000.0f * AUDIO_SAMPLE_RATE * sizeof(short); // the man say 50ms - do it
        aoi->context = myContext;
        myContext->mem = (short*) malloc(memsize);
        myContext->voices = paramsChorus[1].value.i;
        me->begin(myContext->mem,memsize,myContext->voices);
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
      pe = new ParamEditor(display,30,80,260,80);
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0;i<COUNT_OF(paramsChorus);i++)
      {
        pe->display.ShowLabel(paramsChorus[i],i,5,27);
        pe->display.ShowValue(paramsChorus[i],i);
      }
      HookControl(ctrl,0,paramsChorus[0]);
      HookControl(ctrl,1,paramsChorus[1]);
      break;      

    case AudioEditMode::edit:
      {
        result = 1;

        if (millis() > next)
        {
          next = millis() + 10;
        
          if (Scale(paramsChorus[0],ctrl.getPot16(0),0.1f))
            pe->display.ShowValue(paramsChorus[0],0);  
          
          if (Scale(paramsChorus[1],ctrl.getPot16(1)))
            pe->display.ShowValue(paramsChorus[1],1);  
        }
        
        if (encM.getButton())
          exitAt = 1;
        else
        {
          if (exitAt)
          {
            me->begin(nullptr,0,0); // stop the chorus
            free(myContext->mem);
            
            myContext->delayTime = paramsChorus[0].value.f;
            size_t memsize = myContext->delayTime / 1000.0f * AUDIO_SAMPLE_RATE * sizeof(short); 
            myContext->mem = (short*) malloc(memsize);
            memset(myContext->mem,0,memsize); // prevent burst of noise
            myContext->voices = paramsChorus[1].value.i;
            me->begin(myContext->mem,memsize,myContext->voices);
            
            exitAt = 0;
            result = 0;
          }
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
*/
