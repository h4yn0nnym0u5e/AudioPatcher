#if !defined(_PARAMSETTERS_H_)
#define _PARAMSETTERS_H_

#include <MIDI.h>
#include "paramEntry.h"
#include "display.h"
#include "limitedEncoder.h"
#include "apMIDI.h"
#include "editors.h"
#include "Harp_samples.h"

extern LimitedEncoder encM,enc0,enc1,enc2;
extern const ParamChoice velocityShapes[];
extern const int16_t arbWAV_sax[];
extern AudioObjStatic_t objList[];


#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)


extern void HookControl(M5w_8angle& ctrl, int ch, const ParamEntry& pe, ParamValue& pv);
extern bool Scale(const ParamEntry& pe, ParamValue& pv, int16_t raw, float filter);

//==========================================================================
template<class Tctxt>
class FileLoader : public FileBase
{
  public:
    FileLoader(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2,
            AudioPatcherDisplay& d,
            const char* bp, const char* fe,
            mode_e m,
            Tctxt& c
            )
            : FileBase(e0,e1,e2,d,bp,fe,m),
            context(c)
            {}
    virtual ~FileLoader() {};
    Tctxt& context;
    void load(const char* nme)
    { 
      //context.arbWAVloaded = context.arbWAV.load(basePath,filePath,nme,".snd"); 
      context.load(basePath,filePath,nme,fileExtn);
    };
};

class ContextWavetable;
class InstrumentPicker : public FileBase
{
  public:
    InstrumentPicker(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2,
            AudioPatcherDisplay& d,
            ContextWavetable& c
            )
            : FileBase(e0,e1,e2,d,"","",FileBase::mode_e::load),
            context(c)
            {}
    virtual ~InstrumentPicker() {};
    void createFileList(const char* path, mode_e mode);
    void clearFileList(void);
    void loadInstrument(const char* nme);
    
    ContextWavetable& context;
    void load(const char* nme)
    { 
      loadInstrument(nme);
    };
};

//==========================================================================
class SettingsEditor 
{
    int lastRowShown;
  public:
    SettingsEditor(AudioPatcherDisplay& d, 
    AudioPatcherDisplay::Box box,
    //int16_t x, int16_t y, int16_t w, int16_t h,
    size_t n,
    const ParamEntry* ppe, ParamValue* ppv, const ParamPage* ppg) 
    : lastRowShown{0},
      display(d.getInstance()),
      paramCount(n), currentPage(0), params(ppe), aray(ppv), pages(ppg),
      workArea{box}
    {
      display.SaveArea(box.x,box.y,box.w,box.h);
      InitArea();
    }
    ~SettingsEditor() { display.RestoreArea(); }
    
    AudioPatcherDisplay& display;
    size_t paramCount;
    int8_t currentPage;
    const ParamEntry* params;
    ParamValue* aray;
    const ParamPage* pages;
    AudioPatcherDisplay::Box workArea;

    void InitArea(void) { display.InitArea(workArea.x,workArea.y,workArea.w,workArea.h); }
    void Init(const char* title);
    void BlankRow(int row, int16_t yoff) { display.FillRect(workArea.x+1, workArea.y+yoff+row*16, workArea.w-2, 16, EDIT_BKGND); }
    void ShowLabel(int i, int row, int xoff, int yoff) { display.ShowLabel(params[i],aray[i],row,xoff,yoff); }
    void ShowValue(int i) { display.ShowValue(params[i],aray[i],i); }
    void ShowPage(void);
    bool ChangePage(int newPage);
    void ShowVoiceFlag(bool);
};

//==========================================================================
extern SettingsEditor* settingsEditor;
extern M5w_8angle    ctrl;
extern M5w_8encoder  encr;
extern int testExit(uint32_t& exitAt);


#define BOX_DEF(width,lines) \
    (int16_t)(320/2 - (width)/2),(int16_t)(240/2 - (27+(lines)*16+16)/2), \
    (int16_t)(width),(int16_t)(27+(lines)*16+16)

//=====================================================================================
// Read controls and update object's settings accordingly
// \return true if nested setting used "exit" press; don't exit next level up
template <class Tctxt>
bool updateFromControls(Tctxt* myContext, AudioObjInstance* aoi)
{
  size_t pOff = 0, pCount = settingsEditor->paramCount;
  
  if (encM.available()) // attempt to change page
  {
    int oldPage = settingsEditor->currentPage;
    int newPage = oldPage + encM.getValue();
    encM.setValue(0);
    if (settingsEditor->ChangePage(newPage))
    {
      // unhook from old parameter values
      for (int i=0; i < settingsEditor->pages[oldPage].count; i++)
        ctrl.clearHook(i);

      settingsEditor->ShowPage();
    }
  }

  if (nullptr != settingsEditor->pages)
  {
    pOff = settingsEditor->pages[settingsEditor->currentPage].start;
    pCount = settingsEditor->pages[settingsEditor->currentPage].count;
  }
    
  for (size_t i=0; i < pCount; i++)
  {
    int pNum = i + pOff;
    
    if (Scale(settingsEditor->params[pNum],settingsEditor->aray[pNum],ctrl.getPot16(i),0.999f))
    {
      settingsEditor->ShowValue(pNum);
      myContext->setParam(pNum,aoi);
    }
  }

  return false;
}

extern bool updateFromControls(AudioObjInstance* aoi);


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

//=====================================================================================
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

//=====================================================================================
template <class TWaveformCtxt>
void processMIDItoFreqAndAmp(const TWaveformCtxt* ctxt, const MIDIevent* ev, 
                             float& freq, float& ampl)
{
  byte note = ev->note;
      
  note += 12*(ctxt->m.octave.value.i - 4);

  if (0 == ctxt->m.tuning.value.i) // magic: equal temperament
  {
    freq = PatcherVoice::noteToFreq(note);
    freq *= pow(2.0f,ctxt->m.detune.value.f);
    //freq *= ev->pvb.pm.getPitchBend(ctxt->m.PBamount.value.f);
  }
  else
  {
    note += (int) ctxt->m.detune.value.f; // just use another "tonewheel"
    freq = PatcherVoice::noteToFreq(note - 12,notesHammond); // Hammond table is an octave up
  }
  
  switch (velocityShapes[ctxt->m.velocity.value.i].value)
  {
    default: // just in case - gets rid of warning
    case 0: // linear     
      ampl = ev->velocity / 127.0f;
      break;
      
    case 1: // curved     
      ampl = velocity2amplitude[ev->velocity];
      break;
      
    case 2: // as set     
      ampl = ctxt->s.amplitude.value.f;
      break;
      
    case 3: // maximum     
      ampl = 1.0f;
      break;
  }
}

//=====================================================================================
template <class TWaveformCtxt, class TwaveformObject>
void processMIDIforWaveform(AudioObjInstance* aoi, MIDIevent* ev, TWaveformCtxt* ctxt, TwaveformObject* wav)
{
  switch (ev->type)
  {
    case midi::NoteOn:
    {
      float freq, ampl;
      processMIDItoFreqAndAmp(ctxt, ev, freq, ampl);
      ctxt->noteFreq = freq;
      freq *= ev->pvb.pm.getPitchBend(ctxt->m.PBamount.value.f);

      wav->frequency(freq);
      wav->amplitude(ampl);
    }
    break;

    case midi::PitchBend:
      if (0 == ctxt->m.tuning.value.i) // magic: equal temperament
      {
        float freq = ctxt->noteFreq;
        freq *= ev->pvb.pm.getPitchBend(ctxt->m.PBamount.value.f);
        wav->frequency(freq);
      }
      break;
  }
}


template <class TWaveformCtxt, class TwaveformObject>
void processMIDIforKarplusStrong(AudioObjInstance* aoi, MIDIevent* ev, TWaveformCtxt* ctxt, TwaveformObject* wav)
{
  switch (ev->type)
  {
    case midi::NoteOn:
    {
      float freq, ampl;
      processMIDItoFreqAndAmp(ctxt, ev, freq, ampl);
      ctxt->noteFreq = freq;
 
      wav->noteOn(freq,ampl);
    }
    break;

    case midi::NoteOff:
    {
      float freq, ampl;
      processMIDItoFreqAndAmp(ctxt, ev, freq, ampl);
      wav->noteOff(ampl);
    }
      break;
  }
}


template <class TWaveformCtxt, class TwaveformObject>
void processMIDIforWavetable(AudioObjInstance* aoi, MIDIevent* ev, TWaveformCtxt* ctxt, TwaveformObject* wav)
{
  switch (ev->type)
  {
    case midi::NoteOn:
    {
      float freq, ampl;
      processMIDItoFreqAndAmp(ctxt, ev, freq, ampl);
      ctxt->noteFreq = freq;
 
      wav->playFrequency(freq,(uint8_t) (ampl * 127.0f));
    }
    break;

    case midi::NoteOff:
    {
      wav->stop();
    }
      break;

    case midi::PitchBend:
      if (0 == ctxt->m.tuning.value.i) // magic: equal temperament
      {
        float freq = ctxt->noteFreq;
        freq *= ev->pvb.pm.getPitchBend(ctxt->m.PBamount.value.f);
        wav->setFrequency(freq);
      }
      break;
  }
}
//=====================================================================================
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
        myContext = new Tctxt(*aoi);
        aoi->context = myContext;
        if (!aoi->isAcopy) // making a real object
          editSetStreamParams(*aoi);
        // if we make a copy, the caller must deal with that,
        // e.g. by setting the stream parameters, position etc
      }
      break;
      
    case AudioEditMode::destructor:
      delete myContext;
      aoi->context = nullptr;
      break;

    //---------------------------------------------------------------------------------------------------
    case AudioEditMode::enter: // start editing an object's settings
      {
        result = 1; // claimed
        enterEditMode(myContext,aoi);
        
        settingsEditor = new SettingsEditor(display,myContext->box,
                                myContext->paramCount, myContext->params,myContext->aray,myContext->pages);
        settingsEditor->Init(aoi->objP->name);
        next = 0;
      }
      break;      

    case AudioEditMode::edit: // editing an object's settings
      if (millis() >= next)
      {
        next = millis() + 10;
        if (updateFromControls(myContext,aoi)) // update zapped "exit" event
          exitAt = 0;
        result = testExit(exitAt);
      }
      else 
        result = 1;
      break;      

    case AudioEditMode::exit: // finish editing an object's settings
      exitEditMode(myContext,aoi);
      for (size_t i=0; i < myContext->paramCount; i++)
        ctrl.clearHook(i);
      delete settingsEditor;
      settingsEditor = nullptr;
      break;  

    //---------------------------------------------------------------------------------------------------
    case AudioEditMode::MIDIenter: // start editing an object's MIDI settings
      {
        if (nullptr != myContext && nullptr != myContext->MIDIparams) // does it even have any MIDI settings?
        {
          int cols = 1;
          int rows = myContext->MIDIparamCount;

          result = 1; // claimed
          if (myContext->MIDIparamCount > 1 && 0 != myContext->MIDIparams[1].xoff) // multi-column - assume just 2!
            cols = 2;
            
          settingsEditor = new SettingsEditor(display, {BOX_DEF(myContext->box.w,rows / cols)},
                                  myContext->MIDIparamCount, myContext->MIDIparams,myContext->MIDIvalues,nullptr);
  
          settingsEditor->Init(aoi->objP->name);
          settingsEditor->ShowVoiceFlag(aoi->perVoice);
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
          settingsEditor->ShowVoiceFlag(aoi->perVoice);
        }
          
      }
      result = testExit(exitAt);
      break;      

    case AudioEditMode::MIDIexit: // finish editing an object's MIDI settings
      for (size_t i=0; i < myContext->paramCount; i++)
        ctrl.clearHook(i);
      delete settingsEditor;
      settingsEditor = nullptr;
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
//
//   .d8888b.                    888                     888             
//  d88P  Y88b                   888                     888             
//  888    888                   888                     888             
//  888         .d88b.  88888b.  888888 .d88b.  888  888 888888 .d8888b  
//  888        d88""88b 888 "88b 888   d8P  Y8b `Y8bd8P' 888    88K      
//  888    888 888  888 888  888 888   88888888   X88K   888    "Y8888b. 
//  Y88b  d88P Y88..88P 888  888 Y88b. Y8b.     .d8""8b. Y88b.       X88 
//   "Y8888P"   "Y88P"  888  888  "Y888 "Y8888  888  888  "Y888  88888P' 
//                                                                       
//==========================================================================
class ContextBase
{
  public:
    ContextBase(AudioObjInstance& _aoi,
                int nParam, ParamValue* ppv, const ParamEntry* ppe, const ParamPage* ppg = nullptr,
                int nMIDI = 0, ParamValue* pmv = nullptr, const ParamEntry* pme = nullptr) 
      : aoi(_aoi),
        paramCount(nParam),    params(ppe),     aray(ppv),      pages(ppg),
        MIDIparamCount(nMIDI), MIDIparams(pme), MIDIvalues(pmv)
      {}
    virtual ~ContextBase(){}
    virtual void setParam(int i, AudioObjInstance* aoi){}
    virtual void copyParamsTo(ContextBase& dst);

    AudioObjInstance& aoi;
    static constexpr int ARBWAV_PARAM = 0xCAFED00D;
    static constexpr int WTINST_PARAM = ARBWAV_PARAM+1;
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
class ContextBitcrusher  : public ContextBase
{
  public:
    ContextBitcrusher(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.bits, _params) {}
    ~ContextBitcrusher() {}
    static const ParamEntry _params[2];
    struct {ParamValue bits,  sampleRate;} s
                {      {11},  {/* sample rate / */ 8}      }; 
    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(260,COUNT_OF(_params))};

    void enterEditMode(AudioObjInstance* aoi);
    void exitEditMode(AudioObjInstance* aoi);
};


//-----------------------------------------------------------------------------------------
class ContextChorus  : public ContextBase
{
  public:
    ContextChorus(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.length, _params), mem{0}, tmp{0} {}
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
	  static constexpr AudioPatcherDisplay::Box box{BOX_DEF(260,COUNT_OF(_params))};

    void enterEditMode(AudioObjInstance* aoi);
    void exitEditMode(AudioObjInstance* aoi);
};


//-----------------------------------------------------------------------------------------
class ContextFlange  : public ContextBase
{
  public:
    ContextFlange(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.length, _params), mem{0}, tmp{0} {}
    ~ContextFlange() 
    { 
      if (nullptr != mem.ptr) free(mem.ptr); 
    }
    static const ParamEntry _params[4];
    struct {ParamValue length,  offset, depth,  rate;} s
                {      {10.0f}, {0.5f}, {0.25f}, {1}      }; 
    struct memRecord {short* ptr; size_t sz; } mem,tmp;
    void allocMem(memRecord&,size_t,AudioObjInstance*);
    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(230,COUNT_OF(_params))};

    void enterEditMode(AudioObjInstance* aoi);
    void exitEditMode(AudioObjInstance* aoi);
};


//-----------------------------------------------------------------------------------------
class ContextEnvelope : public ContextBase
{
  public:
    ContextEnvelope(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.delay, _params) {}
    static const ParamEntry _params[7];
    struct {ParamValue delay,  attack, hold,   decay,   sustain, release,  releaseNoteOn;} s
                {      {0.0f}, {3.392f},{1.322f}, {5.129f}, {0.5f},  {8.229f}, {2.322f}      };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(240,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
class ContextExpEnvelope : public ContextBase
{
  public:
    ContextExpEnvelope(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.delay, _params) {}
    static const ParamEntry _params[7];
    struct {ParamValue delay,  attack,  hold,     decay,    sustain, release,  shape;} s
                {      {0.0f}, {3.392f},{1.322f}, {5.129f}, {0.5f},  {8.229f}, {0.90f}      };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(240,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
class ContextHammondVibrato : public ContextBase
{
  public:
    ContextHammondVibrato(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.mode, _params) {}
    static const ParamEntry _params[2];
    struct {ParamValue mode, depth;} s
                {      {1},  {1}     };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(180,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
class ContextFreeverb : public ContextBase
{
  public:
    ContextFreeverb(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.roomsize, _params) {}
    static const ParamEntry _params[2];
    struct {ParamValue roomsize, damping;} s
                {      {0.5f},  {0.5f}     };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(180,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
class ContextFreeverbStereo : public ContextBase
{
  public:
    ContextFreeverbStereo(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.roomsize, _params) {}
    static const ParamEntry _params[2];
    struct {ParamValue roomsize, damping;} s
                {      {0.5f},  {0.5f}     };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(180,COUNT_OF(_params))};
};


//-----------------------------------------------------------------------------------------
class ContextReverb : public ContextBase
{
  public:
    ContextReverb(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.reverbTime, _params) {}
    static const ParamEntry _params[1];
    struct {ParamValue reverbTime;} s
                {      {1.0f},       };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(220,COUNT_OF(_params))};
};


//-----------------------------------------------------------------------------------------
class ContextBiquad : public ContextBase
{
  public:
    ContextBiquad(AudioObjInstance& _aoi)
     : ContextBase(_aoi, COUNT_OF(_params), &s.stage, _params) 
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
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(260,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
class ContextLadder : public ContextBase
{
    static const ParamEntry _params[6];
    struct {ParamValue frequency, resonance,octaves,drive,  gain,   interpolation;} s
                   {          {11.0f},   {0.5f},   {2.0f}, {1.0f}, {0.2f}, {1}    };
  public:
    ContextLadder(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.frequency, _params) {}
    ~ContextLadder(){}
    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(270,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
class ContextStateVariable : public ContextBase 
{
  public:
    ContextStateVariable(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.frequency, _params, nullptr,
                                         COUNT_OF(MIDIparams), &m.CCnum, MIDIparams) {}
    static const ParamEntry _params[4];
    struct {ParamValue frequency,resonance,octaves,tracking;} s
                   {      {8.0f}, {0.7f},   {1.0f},  {1.0f} };
    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(260,COUNT_OF(_params))};
    
    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[3];
    struct {ParamValue CCnum, CCmin,  CCmax; } m 
                    {  {1},   {0.0f}, {1.0f}}; // mod wheel, full positive amplitude range
    const float MIN_CUTOFF = 10.0f; // how low can we go?
};

//-----------------------------------------------------------------------------------------
class ContextMixer4 : public ContextBase  
{
  public:
    ContextMixer4(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), gains, _params, nullptr,
                                  COUNT_OF(MIDIparams), CCs, MIDIparams) {}
    static const ParamEntry _params[4];
    ParamValue gains[4]{{0.55f},{0.55f},{0.55f},{0.55f}};
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(120,COUNT_OF(_params))};
    
    void setParam(int i, AudioObjInstance* aoi);

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[4];
    ParamValue CCs[4]{{19},{23},{27},{31}}; // Akai MIDImix volume 1-4;                         
};

//-----------------------------------------------------------------------------------------
class ContextMixer : public ContextBase   
{    
    static const ParamPage _pages[2];
  public:
    ContextMixer(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), gainEtc, _params, _pages) {}
    static const ParamEntry _params[10];
    ParamValue gainEtc[10]{
        {0.55f}, {0.55f}, 
        {0.55f}, {0.55f},
        {0.55f}, {0.55f}, 
        {0.55f}, {0.55f},
        
        {0.55f}, // master gain
        {0.70f}, // soft knee point
    };
    static const int boxWidth{240 + 12};
		static constexpr AudioPatcherDisplay::Box box{BOX_DEF(240 + 12,4)};
    
    void setParam(int i, AudioObjInstance* aoi);
};

//-----------------------------------------------------------------------------------------
#define EDIT_MIXER_STEREO_PAN_OFF ((int) (8*12+20)) // x-offset of pan label
class ContextMixerStereo : public ContextBase   
{    
    static const ParamPage _pages[3];
  public:
    ContextMixerStereo(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), gainOrPan, _params, _pages) {}
    static const ParamEntry _params[20];
    ParamValue gainOrPan[20]{
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},

        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},
        {0.55f},{0.0f},

        {0.55f}, // master gain
        {0.70f}, // soft knee point
        {0.22f}, // pan law
        {0} // pan type
    };
    static const int boxWidth{EDIT_MIXER_STEREO_PAN_OFF + 120 + 12};
		static constexpr AudioPatcherDisplay::Box box{BOX_DEF(EDIT_MIXER_STEREO_PAN_OFF + 120 + 12,4)};
    
    void setParam(int i, AudioObjInstance* aoi);
};

//-----------------------------------------------------------------------------------------
class ContextNoise : public ContextBase
{
  public:
    ContextNoise(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &amplitude, _params) {}
    static const ParamEntry _params[1];
    ParamValue amplitude{0.0f};

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(180,COUNT_OF(_params))};
};

//-----------------------------------------------------------------------------------------
struct WaveformMIDI {ParamValue octave, detune, velocity, tuning, PBamount; };
#define WAVEFORM_MIDI_COUNT (sizeof(WaveformMIDI) / sizeof(ParamValue)) 

template<class Taudio>
class ContextWaveformBase
{
  public:
    ContextWaveformBase() 
      : arbWAVloaded{false} 
      {
        arbWAV.reset();
      }

    /*
      Fix up arbitrary waveform pointer at construction and
      destruction time, so we don't crash or leak memory.

      This context object doesn't have direct access to the 
      stream or AudioObjInstance, so we can't put this code
      in the constructor and destructor.
    */
    void fixArbWAV(Taudio* stream, AudioEditMode mode)
    {
      switch (mode)
      {
        default: break;
    
        case AudioEditMode::constructor:
          stream->arbitraryWaveform(arbWAV.sampleData,10000.0f);
          break;
    
        case AudioEditMode::destructor:
          stream->arbitraryWaveform(nullptr,10000.0f);
          arbWAV.reset();
          arbWAV.sampleData = nullptr; // don't free, it's pointing to default
          break;
      }
    }

    // Provide FileSelector with a load() method
    void load(const char*b, const char*p, const char*l, const char*e)
    {
      arbWAVloaded = arbWAV.load(b,p,l,e); 
    }

    //------ File selection stuff ----------
    const char* const root = "/arbWavs";
    const char* const extn = ".snd";

    //------ Stuff to remember ----------
    float noteFreq; // basic note frequency before modification with pitch bend
    arbWAVrecord<int16_t> arbWAV; // record of arbitrary waveform
    bool arbWAVloaded; // temporary flag while user is seeking a waveform to load
};


//-----------------------------------------------------------------------------------------
// Waveform-like context for use by filter keyboard tracking etc.
class ContextMIDInote : public ContextBase 
{
    AudioObjInstance dummyAOI{objList[0]}; // dummy object: do not use!
  public:
    struct {ParamValue amplitude;} s
                 {       {1.0f}    };

    //------ MIDI settings ----------
    WaveformMIDI m {{4},{0.00f},{0},{0}, {0.0f}};
    ContextMIDInote() 
        : ContextBase(dummyAOI, 1, &s.amplitude, nullptr, nullptr,
                      WAVEFORM_MIDI_COUNT, &m.octave, nullptr) {}
};


//-----------------------------------------------------------------------------------------
class ContextWaveform : public ContextBase, public ContextWaveformBase<AudioSynthWaveform> 
{
  public:
    ContextWaveform(AudioObjInstance& _aoi) : ContextBase(
              _aoi, COUNT_OF(_params), &s.waveform, _params, nullptr,
              COUNT_OF(MIDIparams), &m.octave, MIDIparams),
            fileSelector{nullptr}
    {
      display.GetDefaultKeyboardArea(box.x, box.y, box.w, box.h);
    }
    static const ParamEntry _params[6];
    struct {ParamValue waveform,frequency,amplitude,pulseWidth,offset,arbWAV;} s
                 {     {0},     {7.0f},   {0.5f},   {0.333f},  {0.0f},{&arbWAV}};

    void setParam(int i, AudioObjInstance* aoi);
    AudioPatcherDisplay::Box box;

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[WAVEFORM_MIDI_COUNT];
    WaveformMIDI m {{4},{0.00f},{0},{0}, {0.0f}};

    // File loader
    FileLoader<ContextWaveform>* fileSelector;
};


//-----------------------------------------------------------------------------------------
class ContextWaveformDc : public ContextBase
{
  public:
    ContextWaveformDc(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &amplitude, _params, nullptr, 
                                      COUNT_OF(MIDIparams), &m.CCnum, MIDIparams) {}
    static const ParamEntry _params[1];
    ParamValue amplitude{0.0f};

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(160,COUNT_OF(_params))};
    
    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[3];
    struct {ParamValue CCnum, CCmin,  CCmax; } m 
                    {  {1},   {0.0f}, {1.0f}}; // mod wheel, full positive amplitude range
};


//-----------------------------------------------------------------------------------------
class ContextWaveformModulated 
    : public ContextBase, 
      public ContextWaveformBase<AudioSynthWaveformModulated>
{ 
  public:
    ContextWaveformModulated(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.waveform, _params, nullptr, 
                                            COUNT_OF(MIDIparams), &m.octave, MIDIparams),
                                 fileSelector{nullptr}
    {
      display.GetDefaultKeyboardArea(box.x, box.y, box.w, box.h);
    }
    ~ContextWaveformModulated() 
    {
      if (nullptr != arbWAV.sampleData)
        free(arbWAV.sampleData);
    }
    static const ParamEntry _params[7];
    struct {ParamValue waveform,frequency,amplitude,offset,modType,modDepth,arbWAV;} s 
                            {{0},{7.0f},{0.5f},{0.0f},{0},{1.0f},{&arbWAV}};
    AudioPatcherDisplay::Box box;
          
    void setParam(int i, AudioObjInstance* aoi);

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[WAVEFORM_MIDI_COUNT];
    WaveformMIDI m {{4},{0.00f},{0},{0},{0.0f}};

    // File loader
    FileLoader<ContextWaveformModulated>* fileSelector;
};

//-----------------------------------------------------------------------------------------
class ContextWavetable : public ContextBase
{ 
  public:
    ContextWavetable(AudioObjInstance& _aoi) : ContextBase(_aoi, 
                  COUNT_OF(_params), &s.sf2file, _params, nullptr, 
                  COUNT_OF(MIDIparams), &m.octave, MIDIparams),
                sf2path{nullptr}, sf2file{nullptr}, instName{nullptr},
                instrument{&Harp}, 
                fileSelector{nullptr}, instSelector{nullptr},
                arbWAV{s.index.value.i}
    {
      display.GetDefaultKeyboardArea(box.x, box.y, box.w, box.h); // edit box is file selector sized
      arbWAV.reset();
      aoi.streamP.Wavetable->setInstrument(*arbWAV.sampleData); // set default instrument
    }

    ~ContextWavetable() 
    {
      arbWAV.reset();
      aoi.streamP.Wavetable->setInstrument(*arbWAV.sampleData);
    }

    static const ParamEntry _params[3];
    struct {ParamValue sf2file,  index,amplitude;} s 
                     {{&arbWAV},  {0},   {1.0f} };
    AudioPatcherDisplay::Box box;
          
    void setParam(int i, AudioObjInstance* aoi);

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[WAVEFORM_MIDI_COUNT];
    WaveformMIDI m {{4},{0.00f},{0},{0},{0.0f}};

    //------ Stuff to remember ----------
    float noteFreq; // basic note frequency before modification with pitch bend
    char* sf2path, *sf2file, *instName;
    const AudioSynthWavetable::instrument_data* instrument; // record of aritrary waveform

    // File loader
    const char* const root = "/soundfonts";
    const char* const extn = ".sf2";
    FileLoader<ContextWavetable>* fileSelector;
    InstrumentPicker* instSelector;
    // Provide FileSelector with a load() method
    void load(const char*b, const char*p, const char*l, const char*e)
    {
      arbWAVloaded = arbWAV.load(b,p,l,e); 
    }

    //------ Stuff to remember ----------
    arbWAVrecord<AudioSynthWavetable::instrument_data> arbWAV; // record of wavetable
    bool arbWAVloaded; // temporary flag while user is seeking a waveform to load
};

//-----------------------------------------------------------------------------------------
class ContextKarplusStrong : public ContextBase
{ 
  public:
    ContextKarplusStrong(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.frequency, _params, nullptr, 
                                         COUNT_OF(MIDIparams), &m.octave, MIDIparams) {}
    static const ParamEntry _params[2];
    struct {ParamValue frequency,amplitude;} s {{7.0f},{0.5f}};
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(260,COUNT_OF(_params))};
          
    void setParam(int i, AudioObjInstance* aoi);

    //------ MIDI settings ----------
    static const ParamEntry MIDIparams[WAVEFORM_MIDI_COUNT];
    WaveformMIDI m {{4},{0.00f},{0},{0},{0.0f}};
    float noteFreq; // basic note frequency before modification with pitch bend
};

//-----------------------------------------------------------------------------------------
class ContextControlSGTL5000 : public ContextBase
{ 
  public:
    ContextControlSGTL5000(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.volume, _params) {}
    static const ParamEntry _params[6];
    struct {ParamValue volume,  inputSelect,micGain,autoVolume,lineInLevel,lineOutLevel;} s
                 {             {0.25f}, {0},        {20},   {0},       {10},       {11}           };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(260,COUNT_OF(_params))};
};
     

//-----------------------------------------------------------------------------------------
class ContextDexed : public ContextBase
{
  public:
  ContextDexed(AudioObjInstance& _aoi) : ContextBase(_aoi, COUNT_OF(_params), &s.reverbTime, _params) {}
    static const ParamEntry _params[1];
    struct {ParamValue reverbTime;} s
                {      {1.0f},       };

    void setParam(int i, AudioObjInstance* aoi);
    static constexpr AudioPatcherDisplay::Box box{BOX_DEF(220,COUNT_OF(_params))};
};


#endif // !defined(_PARAMSETTERS_H_)
