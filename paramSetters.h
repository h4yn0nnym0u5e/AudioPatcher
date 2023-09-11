#if !defined(_PARAMSETTERS_H_)
#define _PARAMSETTERS_H_

#include <MIDI.h>
#include "paramEntry.h"
#include "display.h"
#include "LimitedEncoder.h"
#include "apMIDI.h"

extern LimitedEncoder encM,enc0,enc1,enc2;

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)

/*
extern bool ScaleF(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter);
extern bool ScaleFreq(const ParamEntry& pe, ParamValue& pv, int16_t raw, int16_t pow2, float filter);
extern ;
extern ;
extern ;
extern ;
extern ;
*/
extern void HookControl(M5w_8angle& ctrl, int ch, const ParamEntry& pe, ParamValue& pv);
extern bool Scale(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter);


//==========================================================================
class SettingsEditor 
{
  public:
    SettingsEditor(AudioPatcherDisplay& d, 
    int16_t x, int16_t y, int16_t w, int16_t h,
    size_t n,
    const ParamEntry* ppe, ParamValue* ppv, const ParamPage* ppg) 
    : display(d.getInstance()),
      paramCount(n), currentPage(0), params(ppe), aray(ppv), pages(ppg),
      workArea{.x=x, .y=y, .w=w, .h=h}
    {
      display.SaveArea(x,y,w,h);
      display.InitArea(x,y,w,h);
    }
    ~SettingsEditor() { display.RestoreArea(); }
    
    AudioPatcherDisplay& display;
    size_t paramCount;
    int8_t currentPage;
    const ParamEntry* params;
    ParamValue* aray;
    const ParamPage* pages;
    struct {int16_t x,y,w,h;} workArea;

    void Init(const char* title);
    void ShowLabel(int i, int row, int xoff, int yoff) { display.ShowLabel(params[i],aray[i],row,xoff,yoff); }
    void ShowValue(int i) { display.ShowValue(params[i],aray[i],i); }
    void ShowPage(void);
    bool ChangePage(int newPage);
    void ShowVoiceFlag(bool);
};

//==========================================================================
extern SettingsEditor* se;
extern M5w_8angle    ctrl;
extern M5w_8encoder  encr;
extern int testExit(uint32_t& exitAt);


#define BOX_DEF(width,lines) (320/2 - (width)/2),(240/2 - (27+(lines)*16+16)/2),(width),(27+(lines)*16+16)

//=====================================================================================
template <class Tctxt>
void updateFromControls(Tctxt* myContext, AudioObjInstance* aoi)
{
  size_t pOff = 0, pCount = se->paramCount;
  
  if (encM.available()) // attempt to change page
  {
    int oldPage = se->currentPage;
    int newPage = oldPage + encM.getValue();
    encM.setValue(0);
    if (se->ChangePage(newPage))
    {
      // unhook from old parameter values
      for (int i=0; i < se->pages[oldPage].count; i++)
        ctrl.clearHook(i);

      se->ShowPage();
    }
  }

  if (nullptr != se->pages)
  {
    pOff = se->pages[se->currentPage].start;
    pCount = se->pages[se->currentPage].count;
  }
    
  for (size_t i=0; i < pCount; i++)
  {
    int pNum = i + pOff;
    
    if (Scale(se->params[pNum],se->aray[pNum],ctrl.getPot16(i),0.999f))
    {
      se->ShowValue(pNum);
      myContext->setParam(pNum,aoi);
    }
  }
}

extern void updateFromControls(AudioObjInstance* aoi);


//=====================================================================================
extern int editGetParamsAny(const ParamEntry* params, const ParamValue* aray, const size_t paramCount, getSetParams* p);
template <class Tctxt>
int editGetParams(AudioObjInstance* aoi, getSetParams* p)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  return editGetParamsAny(myContext->params, myContext->aray, myContext->paramCount, p);
}

template <class Tctxt>
int editGetMIDIparams(AudioObjInstance* aoi, getSetParams* p)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  int result;
  
  result = editGetParamsAny(myContext->MIDIparams, myContext->MIDIvalues, myContext->MIDIparamCount, p);

  return result;
}

//=====================================================================================
extern int editSetParamsAny(const ParamEntry* params, ParamValue* aray, const size_t paramCount, getSetParams* p);
extern int editSetStreamParams(AudioObjInstance& aoi);

//=====================================================================================
// Set object's parameters from supplied string
template <class Tctxt>
int editSetParams(AudioObjInstance* aoi, getSetParams* p)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  int result = editSetParamsAny(myContext->params, myContext->aray, myContext->paramCount, p);
  
  editSetStreamParams(*aoi);
  
  return result;
}

// Set object's MIDI parameters from supplied string
template <class Tctxt>
int editSetMIDIparams(AudioObjInstance* aoi, getSetParams* p)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  
  int result = editSetParamsAny(myContext->MIDIparams, myContext->MIDIvalues, myContext->MIDIparamCount, p);
  
  return result;
}

//=====================================================================================
template <class Tctxt>
void processMIDIevent(AudioObjInstance* aoi, MIDIevent* ev){} // no special action for most AudioStream classes
template <class Tctxt>
int isActive(AudioObjInstance* aoi){ return 0; } // most AudioStream classes don't have "active" state
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
        if (!aoi->isAcopy) // making a real object
        {       
          myContext = new Tctxt;
          aoi->context = myContext;
          editSetStreamParams(*aoi);
        }
        // if we make a copy, the caller must deal with that,
        // e.g. by setting the stream parameters, position etc
      }
      break;
      
    case AudioEditMode::destructor:
      if (!aoi->isAcopy) // only delete context if it's the real deal
      {
        delete myContext;
      }
      break;

    //---------------------------------------------------------------------------------------------------
    case AudioEditMode::enter: // start editing an object's settings
      {
        int cols = 1;
        
        result = 1; // claimed
        enterEditMode(myContext,aoi);
        if (myContext->paramCount > 1 && 0 != myContext->params[1].xoff) // multi-column - assume just 2!
          cols = 2;
          
        se = new SettingsEditor(display,BOX_DEF(Tctxt::boxWidth,myContext->paramCount / cols),
                                myContext->paramCount, myContext->params,myContext->aray,myContext->pages);
        se->Init(aoi->objP->name);
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
      delete se;
      se = nullptr;
      break;  

    //---------------------------------------------------------------------------------------------------
    case AudioEditMode::MIDIenter: // start editing an object's MIDI settings
      {
        if (nullptr != myContext && nullptr != myContext->MIDIparams) // does it even have any MIDI settings?
        {
          int cols = 1;
          
          result = 1; // claimed
          if (myContext->MIDIparamCount > 1 && 0 != myContext->MIDIparams[1].xoff) // multi-column - assume just 2!
            cols = 2;
            
          se = new SettingsEditor(display,BOX_DEF(Tctxt::boxWidth,myContext->MIDIparamCount / cols),
                                  myContext->MIDIparamCount, myContext->MIDIparams,myContext->MIDIvalues,nullptr);
  
          se->Init(aoi->objP->name);
          se->ShowVoiceFlag(aoi->perVoice);
          enc0.setValue(aoi->perVoice);
          next = 0;
        }
        else // no parameters, might toggle per-voice
        {
          editTogglePerVoice(aoi);
        }
      }
      break;      

    case AudioEditMode::MIDIedit: // editing an object's MIDI settings
      if (millis() >= next)
      {
        next = millis() + 10;
        updateFromControls(aoi);
        if (enc0.available())
        {
          aoi->perVoice = enc0.getValue() & 1;
          se->ShowVoiceFlag(aoi->perVoice);
        }
          
      }
      result = testExit(exitAt);
      break;      

    case AudioEditMode::MIDIexit: // finish editing an object's MIDI settings
      for (size_t i=0; i < myContext->paramCount; i++)
        ctrl.clearHook(i);
      delete se;
      se = nullptr;
      display.DrawPerVoice(*aoi); // in case it was changed
      break;  

          
    //---------------------------------------------------------------------------------------------------
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
      
    //---------------------------------------------------------------------------------------------------
    case AudioEditMode::getMIDIparams: // create a string with the object's MIDI settings
      {
        result = editGetMIDIparams<Tctxt>(aoi,(getSetParams*) params);
      }
      break;

    // patch L: ~2: 4,8.774944,0.226114,0.294576,0.000000,      
    case AudioEditMode::setMIDIparams: // load the object's MIDI settings from a string
      {
        editSetMIDIparams<Tctxt>(aoi,(getSetParams*) params);
        result = 1;
      }
      break;

    case AudioEditMode::processMIDIevent:
      processMIDIevent<Tctxt>(aoi,(MIDIevent*) params);
      break;      

    case AudioEditMode::checkIfActive:
      result = isActive<Tctxt>(aoi);
      break;      

    //---------------------------------------------------------------------------------------------------
    default:
      break;      
  }
  return result;  
}
//==========================================================================
class ContextBase
{
  public:
    ContextBase(int nParam, ParamValue* ppv, const ParamEntry* ppe, const ParamPage* ppg = nullptr,
                int nMIDI = 0, ParamValue* pmv = nullptr, const ParamEntry* pme = nullptr) 
      : paramCount(nParam),    params(ppe),     aray(ppv),      pages(ppg),
        MIDIparamCount(nMIDI), MIDIparams(pme), MIDIvalues(pmv)
      {}
    virtual ~ContextBase(){}
    virtual void setParam(int i, AudioObjInstance* aoi){}

    // ------ stream object settings ----------
    const size_t paramCount;
    const ParamEntry* params;
    ParamValue* aray;
    const ParamPage* pages;

    //------ MIDI settings ----------
    const size_t MIDIparamCount;
    const ParamEntry* MIDIparams;
    ParamValue* MIDIvalues;    
};

//==========================================================================
class ContextChorus  : public ContextBase
{
  public:
    ContextChorus() : ContextBase(COUNT_OF(_params), &s.length, _params), mem{0}, tmp{0} {}
    ~ContextChorus() 
    { 
      if (nullptr != mem.ptr) free(mem.ptr); 
    }
    static const ParamEntry _params[2];
    struct {ParamValue length,  voices;} s
                {             {50.0f}, {2}      }; 
    struct memRecord {short* ptr; size_t sz; } mem,tmp;
    void allocMem(memRecord&,size_t,AudioObjInstance*);
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};

    void enterEditMode(AudioObjInstance* aoi);
    void exitEditMode(AudioObjInstance* aoi);
};

class ContextEnvelope : public ContextBase
{
  public:
    ContextEnvelope() : ContextBase(COUNT_OF(_params), &s.delay, _params) {}
    static const ParamEntry _params[7];
    struct {ParamValue delay,  attack, hold,   decay,   sustain, release,  releaseNoteOn;} s
                {      {0.0f}, {3.392f},{1.322f}, {5.129f}, {0.5f},  {8.229f}, {2.322f}      };

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{270};
};

class ContextBiquad : public ContextBase
{
  public:
    ContextBiquad()
     : ContextBase(COUNT_OF(_params), &s.stage, _params) 
    {
      for (size_t i = 0;i<4;i++)
      {
        for (size_t j=1;j<COUNT_OF(_params);j++)
          stageSettings[i][j] = aray[j].value;
        stageSettings[i][0].i = i; // set stage numbers correctly
      }
    }
    static const ParamEntry _params[6];
    
struct {ParamValue  stage,response,frequency,     Q,        gain,   slope;} s 
                        {           {0},  {1},     {9.64385618f}, {0.707f}, {0.8f}, {1.0f} };   
    ValUnion stageSettings[4][COUNT_OF(_params)];
    int prevStage{0};
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
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
};

class ContextStateVariable : public ContextBase 
{
  public:
    ContextStateVariable() : ContextBase(COUNT_OF(_params), &s.frequency, _params) {}
    static const ParamEntry _params[3];
    struct {ParamValue frequency,resonance,octaves;} s
                   {          {8.0f}, {0.7f},   {1.0f}    };
    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
};

class ContextMixer4 : public ContextBase  
{
  public:
    ContextMixer4() : ContextBase(COUNT_OF(_params), gains, _params){}
    static const ParamEntry _params[4];
    ParamValue gains[4]{{0.55f},{0.55f},{0.55f},{0.55f}};
    static const int boxWidth{120};
    
    void setParam(int i, AudioObjInstance* aoi);
};

#define EDIT_MIXER_STEREO_PAN_OFF ((int) (8*12+20))
class ContextMixerStereo : public ContextBase   
{    
    static const ParamPage _pages[2];
  public:
    ContextMixerStereo() : ContextBase(COUNT_OF(_params), gainOrPan, _params, _pages) {}
    static const ParamEntry _params[8];
    ParamValue gainOrPan[8]{
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},
    };
    static const int boxWidth{EDIT_MIXER_STEREO_PAN_OFF + 120 + 12};
    
    void setParam(int i, AudioObjInstance* aoi);
};

class ContextNoise : public ContextBase
{
  public:
    ContextNoise() : ContextBase(COUNT_OF(_params), &amplitude, _params) {}
    static const ParamEntry _params[1];
    ParamValue amplitude{0.0f};

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{180};
};

class ContextWaveform : public ContextBase 
{
  public:
    ContextWaveform() : ContextBase(COUNT_OF(_params), &s.waveform, _params, nullptr,
                                            COUNT_OF(MIDIparams), &m.octave, MIDIparams) {}
    static const ParamEntry _params[5];
    struct {ParamValue waveform,frequency,amplitude,pulseWidth,offset;} s
                 {             {0},     {7.0f},   {0.5f},   {0.333f},  {0.0f}    };

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[4];
    struct {ParamValue octave,detune,velocity,tuning;} m {{4},{0.00f},{0},{0}};
};

class ContextWaveformDc : public ContextBase
{
  public:
    ContextWaveformDc() : ContextBase(COUNT_OF(_params), &amplitude, _params) {}
    static const ParamEntry _params[1];
    ParamValue amplitude{0.0f};

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{160};
};

class ContextWaveformModulated : public ContextBase
{ 
  public:
    ContextWaveformModulated() : ContextBase(COUNT_OF(_params), &s.waveform, _params, nullptr, 
                                            COUNT_OF(MIDIparams), MIDIvalues, MIDIparams) {}
    static const ParamEntry _params[6];
    struct {ParamValue waveform,frequency,amplitude,offset,modType,modDepth;} s {{0},{7.0f},{0.5f},{0.0f},{0},{1.0f}};
    static const int boxWidth{260};
          
    void setParam(int i, AudioObjInstance* aoi);

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[4];
    ParamValue MIDIvalues[COUNT_OF(MIDIparams)]{{4},{0.00f},{0},{0}};
};

class ContextControlSGTL5000 : public ContextBase
{ 
  public:
    ContextControlSGTL5000() : ContextBase(COUNT_OF(_params), &s.volume, _params) {}
    static const ParamEntry _params[6];
    struct {ParamValue volume,  inputSelect,micGain,autoVolume,lineInLevel,lineOutLevel;} s
                 {             {0.25f}, {0},        {20},   {0},       {10},       {11}           };

    void setParam(int i, AudioObjInstance* aoi);
    static const int boxWidth{260};
};
     

#endif // !defined(_PARAMSETTERS_H_)
