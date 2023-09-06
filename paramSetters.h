#if !defined(_PARAMSETTERS_H_)
#define _PARAMSETTERS_H_

#include "paramEntry.h"
#include "display.h"
#include "M5wire.h"

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)

class ParamEditor 
{
  public:
    ParamEditor(AudioPatcherDisplay& d, 
    int16_t x, int16_t y, int16_t w, int16_t h) 
    : display(d.getInstance()),
      workArea{.x=x, .y=y, .w=w, .h=h}
    {
      display.SaveArea(x,y,w,h);
      display.InitArea(x,y,w,h);
    }
    ~ParamEditor() { display.RestoreArea(); }
    
    AudioPatcherDisplay& display;
    struct {int16_t x,y,w,h;} workArea;
};

//==========================================================================
extern ParamEditor* pe;
extern M5w_8angle    ctrl;
extern M5w_8encoder  encr;
extern int testExit(uint32_t& exitAt);


#define BOX_DEF(width,lines) (320/2 - (width)/2),(240/2 - (27+(lines)*16+16)/2),(width),(27+(lines)*16+16)

//=====================================================================================
template <class Tctxt>
void updateFromControls(Tctxt* myContext, AudioObjInstance* aoi)
{
  for (size_t i=0; i < myContext->paramCount; i++)
  {
    if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
    {
      pe->display.ShowValue(myContext->params[i],myContext->aray[i],i);
      myContext->setParam(i,aoi);
    }
  }
}

//=====================================================================================
template <class Tctxt>
int editGetParams(AudioObjInstance* aoi, getSetParams* p)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  size_t left = p->sz;
  char* ptr = p->buffer;
  int off = 0;
  
  for (size_t i=0; i < myContext->paramCount && off >= 0 && left >= 15; i++)
  {
    switch (myContext->params[i].ValType)
    {
      case 'l':
      case 'f': off = sprintf(ptr,"%f,",myContext->aray[i].value.f); break;
      case 'c':
      case 'i': off = sprintf(ptr,"%d,",myContext->aray[i].value.i); break;
    }

    ptr += off;
    left -= off;            
  }
  p->sz = ptr - p->buffer; // amount of buffer used

  return 1;
}


//=====================================================================================
// Set object's parameters from supplied string
template <class Tctxt>
int editSetParams(AudioObjInstance* aoi, getSetParams* p)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  char* ptr = p->buffer;
  int off = 0;
  for (size_t i=0; i < myContext->paramCount && off >= 0; i++)
  {
    ValUnion value;
    
    switch (myContext->params[i].ValType)
    {
      case 'f':
      case 'l':
        sscanf(ptr,"%f,%n",&value.f,&off);
        if (value.f < myContext->params[i].min.f || value.f > myContext->params[i].max.f)
          value.f = (myContext->params[i].min.f + myContext->params[i].max.f) / 2.0f;
        Serial.printf("%s = %.3f ... ",myContext->params[i].label,value.f);
        myContext->aray[i].value.f = value.f;
        break;
        
      case 'i':
      case 'c':
        sscanf(ptr,"%d,%n",&value.i,&off);
        if (value.i < myContext->params[i].min.i || value.i > myContext->params[i].max.i)
          value.i = (myContext->params[i].min.i + myContext->params[i].max.i) / 2;
        Serial.printf("%s = %d ... ",myContext->params[i].label,value.i);
        myContext->aray[i].value.i = value.i;
        break;
    }
    myContext->setParam(i,aoi);

    ptr += off;
  }
  Serial.println();
  p->sz = ptr - p->buffer;
  return 1;
}


//=====================================================================================
template <class Tctxt>
void enterEditMode(Tctxt* myContext, AudioObjInstance* aoi){} // no special action for most AudioStream classes
//=====================================================================================
template <class Tctxt>
void exitEditMode(Tctxt* myContext, AudioObjInstance* aoi){} // no special action for most AudioStream classes
//=====================================================================================
template <class Tstream, class Tctxt>
int editObjType(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  //Tstream* me = (Tstream*) aoi->streamP.streamObj;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new Tctxt;
        aoi->context = myContext;
        for (size_t i=0; i < myContext->paramCount; i++)
          myContext->setParam(i,aoi); // send initial settings to AudioStream object
      }
      break;
      
    case AudioEditMode::destructor:
      delete myContext;
      break;

    case AudioEditMode::enter: // start editing an object's settings
      {
        int row = 0, cols = 1;
        
        result = 1; // claimed
        enterEditMode(myContext,aoi);
        if (myContext->paramCount > 1 && 0 != myContext->params[1].xoff) // multi-column - assume just 2!
          cols = 2;
          
        pe = new ParamEditor(display,BOX_DEF(Tctxt::boxWidth,myContext->paramCount / cols));
        pe->display.ShowTitle(aoi->objP->name,5,5);
        for (size_t i=0; i < myContext->paramCount; i++)
        {
          if (0 != myContext->params[i].xoff) /// if we have an X offset
            row--; // we're on the same row as before
          pe->display.ShowLabel(myContext->params[i],myContext->aray[i],row,5,27);
          pe->display.ShowValue(myContext->params[i],myContext->aray[i],row);
          if (!ctrl.isHooking(i)) // specialized enterEditMode() may have hooked already - don't re-do
            HookControl(ctrl,i,myContext->params[i],myContext->aray[i]);
          row++;            
        }
        next = 0;
      }
      break;      

    case AudioEditMode::edit: // editing an object's settings
      if (millis() >= next)
      {
        next = millis() + 10;
        updateFromControls(myContext,aoi);
      }
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit: // finish editing an object's settings
      exitEditMode(myContext,aoi);
      for (size_t i=0; i < myContext->paramCount; i++)
        ctrl.clearHook(i);
      delete pe;
      pe = nullptr;
      break;  
          
    case AudioEditMode::getParams: // create a string with the object's settings
      {
        editGetParams<Tctxt>(aoi,(getSetParams*) params);
        result = 1;
      }
      break;

    // patch L: ~2: 4,8.774944,0.226114,0.294576,0.000000,      
    case AudioEditMode::setParams: // load the object's settings from a string
      {
        editSetParams<Tctxt>(aoi,(getSetParams*) params);
        result = 1;
      }
      break;
      
    default:
      break;      
  }
  return result;  
}
//==========================================================================
class ContextBase
{
  public:
    ContextBase(int nParam, ParamValue* ppv, const ParamEntry* ppe) 
      : paramCount(nParam), aray(ppv), params(ppe)
      {}
    virtual ~ContextBase(){}
    virtual void setParam(int i, AudioObjInstance* aoi){}
    const size_t paramCount;
    ParamValue* aray;
    const ParamEntry* params;
};

//==========================================================================
class ContextChorus 
{
  public:
    ContextChorus() : mem{0}, tmp{0} {}
    ~ContextChorus() { if (nullptr != mem.ptr) free(mem.ptr); }
    static const ParamEntry params[2];
    union {struct {ParamValue length,  voices;} s
                {             {50.0f}, {2}      }; 
                   ParamValue aray[2];
          };
    struct memRecord {short* ptr; size_t sz; } mem,tmp;
    void allocMem(memRecord&,size_t,AudioObjInstance*);
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
    static const int paramCount{COUNT_OF(params)};

    void enterEditMode(AudioObjInstance* aoi);
    void exitEditMode(AudioObjInstance* aoi);
};

class ContextBiquad 
{
  public:
    ContextBiquad()
    {
      for (size_t i = 0;i<4;i++)
      {
        for (size_t j=1;j<COUNT_OF(params);j++)
          stageSettings[i][j] = aray[j].value;
        stageSettings[i][0].i = i; // set stage numbers correctly
      }
    }
    static const ParamEntry params[6];
    
    union {struct {ParamValue  stage,response,frequency,     Q,        gain,   slope;} s 
                        {           {0},  {1},     {9.64385618f}, {0.707f}, {0.8f}, {1.0f} };   
              ParamValue aray[COUNT_OF(params)];
          };
    ValUnion stageSettings[4][COUNT_OF(params)];
    int prevStage{0};
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
    static const int paramCount{COUNT_OF(params)};   
};

class ContextLadder : public ContextBase
{
    static const ParamEntry _params[6];
    struct {ParamValue frequency, resonance,octaves,drive,  gain,   interpolation;} s
                   {          {11.0f},   {0.5f},   {2.0f}, {1.0f}, {0.2f}, {1}    };
  public:
    ContextLadder() : ContextBase(COUNT_OF(_params), &s.frequency, _params) {}
    ~ContextLadder(){}
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{270};
    //static const int paramCount{COUNT_OF(params)};
};

class ContextStateVariable 
{
  public:
  ContextStateVariable(){}
    static const ParamEntry params[3];
    union {struct {ParamValue frequency,resonance,octaves;} s
                   {          {8.0f}, {0.7f},   {1.0f}    };
                   ParamValue aray[3];};
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
    static const int paramCount{COUNT_OF(params)};
};

class ContextMixer4  
{
  public:
    ContextMixer4(){}
    static const ParamEntry params[4];
    ParamValue aray[4]{{0.55f},{0.55f},{0.55f},{0.55f}};
    static const int boxWidth{120};
    
    void setParam(int i, AudioObjInstance* aoi);
    static const int paramCount{COUNT_OF(params)};
};

#define EDIT_MIXER_STEREO_PAN_OFF ((int) (8*12+20))
class ContextMixerStereo  
{
  public:
    ContextMixerStereo(){}
    static const ParamEntry params[8];
    ParamValue aray[8]{
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},
    };
    static const int boxWidth{EDIT_MIXER_STEREO_PAN_OFF + 120 + 12};
    
    void setParam(int i, AudioObjInstance* aoi);
    static const int paramCount{COUNT_OF(params)};
};

class ContextNoise 
{
  public:
    ContextNoise(){}
    static const ParamEntry params[1];
    ParamValue amplitude{0.0f}, *aray{&amplitude};

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{180};
    static const int paramCount{COUNT_OF(params)};   
};

class ContextWaveform {
  public:
    ContextWaveform(){}
    static const ParamEntry params[5];
    union { struct {ParamValue waveform,frequency,amplitude,pulseWidth,offset;} s
                 {             {0},     {7.0f},   {0.5f},   {0.333f},  {0.0f}    };
            ParamValue aray[COUNT_OF(params)];
          };
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
    static const int paramCount{COUNT_OF(params)};
};

class ContextWaveformDc 
{
  public:
    ContextWaveformDc(){}
    static const ParamEntry params[1];
    ParamValue amplitude{0.0f}, *aray{&amplitude};

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{160};
    static const int paramCount{COUNT_OF(params)};   
};

class ContextWaveformModulated 
{ 
  public:
    ContextWaveformModulated() {}
    static const ParamEntry params[6];
    union { struct {ParamValue waveform,frequency,amplitude,offset,modType,modDepth;} s {{0},{7.0f},{0.5f},{0.0f},{0},{1.0f}};
            ParamValue aray[COUNT_OF(params)];
          };
    static const int boxWidth{260};
          
    void setParam(int i, AudioObjInstance* aoi);
    static const int paramCount{COUNT_OF(params)};       
};

class ContextControlSGTL5000 
{ 
  public:
    ContextControlSGTL5000(){}
    static const ParamEntry params[6];
    union { struct {ParamValue volume,  inputSelect,micGain,autoVolume,lineInLevel,lineOutLevel;} s
                 {             {0.25f}, {0},        {20},   {0},       {10},       {11}           };
            ParamValue aray[COUNT_OF(params)];};

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
    static const int paramCount{COUNT_OF(params)};          
};
     

#endif // !defined(_PARAMSETTERS_H_)
