#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

#include <sf22aswt.h>
#include <vector>
#include <algorithm>

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)

#define DEXED_ARBWAV_SIG int16_t, 4104/2

extern void makeFFP(char* buf, const char* base, const char* path, const char* leaf, const char* ext);

extern const int16_t arbWAV_sax[];
extern const uint8_t fmpiano_sysex[];

class arbWAVrecordBase
{
  public:
    arbWAVrecordBase() 
      : path{nullptr}, recSize{0}, loaded{false} 
      {}
    char* path;     // where it was loaded from
    size_t recSize; // space allocated
    bool loaded;

    // Load arbitrary waveform using separate path elements
    bool load(const char* base, const char* path, const char* nme, const char* extn)
    {
      char buf[100];

      makeFFP(buf,base,path,nme,extn);
      return load(buf);
    }

    bool loadData(const char* fname, size_t space,int16_t*& mem, char*& path, size_t& spaceNeeded, const int16_t* theDefault);
    void resetData(int16_t* theDefault, void* sampleData);

    // Load arbitrary waveform using complete file path
    virtual bool load(const char* buf) = 0;
    virtual char* prepare(size_t pathLen) = 0;
    virtual void reset(void) = 0; // reset to default waveform
    virtual bool isDefault(void) = 0;
};

template<typename Tdata, size_t _count = 256>
class arbWAVrecord : public arbWAVrecordBase
{
    static const int ARB_WAV_SAMPLES = _count;
    void* defaultSample;
  public:
    arbWAVrecord() 
      : sampleData{nullptr}
      {
        switch (ARB_WAV_SAMPLES)
        {
          default:
            defaultSample = nullptr;
            break;
          
          case 256: // arbitrary waveform
            defaultSample = (void*) arbWAV_sax;
            break;

          case 4104/2: // FM patch
            defaultSample = (void*) (&fmpiano_sysex[0]);
            break;
        }
        
      }

    Tdata* sampleData; // actual 256-sample data block or wavetable instrument

    using arbWAVrecordBase::load;
    bool load(const char* buf) override;
    void reset(void) override; // reset to default waveform
    bool isDefault(void) override {return (void*) sampleData == defaultSample; };
    void setAll(Tdata* s, char* p, size_t sz, bool l)
    {
      sampleData = s; 
      path = p;
      recSize = sz;
      loaded = l;
    }

    /*
    * Load arbitrary waveform if it's not already been done
    * \returns true if it's available for use
    */
    bool loadIfNeeded(void)
    {
      if (!loaded
        && path != nullptr) // file hasn't been loaded
      {
        if (load(path))
          Serial.printf("Set arbWAV from %s to %08X -> %08X; fingerprint %04.4X,%04.4X\n",
                       path, this, sampleData,
                       ((uint32_t) sampleData[0]) & 0xFFFF, 
                       ((uint32_t) sampleData[1]) & 0xFFFF);
      }
      return loaded && nullptr != sampleData;
    }

    /*
     * Prepare memory to store waveform and its source path
     * \return pointer to memory; may be nullptr
     */
    char* prepare(size_t pathLen)
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
};


class instEntry
{
  public:
    instEntry(int idx, String nm) 
      : index(idx), name(nm), loaded(false)
      {}

    int index;
    String name;
    bool loaded;
};

class instEntryPtr
{
  public:
    instEntry* p;
};
bool operator<(const instEntryPtr& lhs, const instEntryPtr& rhs );


/*
 * Specialize arbWAVrecord for SF2 wavetables
 */
template<>
class arbWAVrecord<AudioSynthWavetable::instrument_data> 
  : public arbWAVrecordBase
{
  public:
    arbWAVrecord(int& ri) 
      : rIndex{ri}, sampleData{nullptr}
      {}

      int& rIndex;       // index in instrument file (SF2 only)
      int  getIndex(void)  { return rIndex; }
      void setIndex(int n) { rIndex = n; }
  
    SF22ASWTreader sf22aswt;
    AudioSynthWavetable::instrument_data* sampleData; // actual wavetable instrument
    std::vector<instEntryPtr> instList;


    using arbWAVrecordBase::load;
    bool load(const char* buf) override; // set the SF2 file path
    bool loadInstrument(void)
    {
      loaded = sf22aswt.Load_instrument(getIndex(), sampleData);
      return loaded;
    }
    char* prepare(size_t pathLen) override;
    void reset(void) override; // reset to default waveform
    bool isDefault(void) override;
    void setAll(AudioSynthWavetable::instrument_data* s, 
                char* p, size_t sz, bool l, int i = -1)
    {
      sampleData = s; 
      path = p;
      setIndex(i);
      recSize = sz;
      loaded = l;
    }
    bool isValid(void) { return sf22aswt.getLastReadWasOK(); }

    /*
    * Load arbitrary waveform if it's not already been done
    * \returns true if it's available for use
    */
    bool loadIfNeeded(void)
    {
      if (!loaded
        && path != nullptr) // file hasn't been loaded
      {
        if ((isValid() || sf22aswt.ReadFile(path)) &&
            load(path) && loadInstrument())
        {
            uint16_t* ptr = (uint16_t*) sampleData;
            if (nullptr != sampleData)
              Serial.printf("Set wavetable from %s:%d to %08X -> %08X; fingerprint %04.4X,%04.4X\n",
                          path, getIndex(),
                          this, (uint32_t) ptr,
                          ((uint32_t) ptr[0]) & 0xFFFF, 
                          ((uint32_t) ptr[1]) & 0xFFFF);
            else
              Serial.printf("Load from %s index %d failed\n", path, getIndex());
        }
      }
      return loaded && nullptr != sampleData;
    }

    int idx; // used ONLY when scanning instrument list
    void instListAddEntry(SF22ASWT::inst_rec& inst); // add entry to list
    void getInstrumentList(void); // populate the instrument list
    void emptyInstrumentList(void); // remove instrument list from emeory
};


template<> void arbWAVrecord<int16_t>::reset(void);
template<> void arbWAVrecord<DEXED_ARBWAV_SIG>::reset(void);

class ParamChoice
{
  public:
    const char* const text;
    const int value;
};

union ValUnion {int i; float f; char* s; 
                arbWAVrecord<int16_t>* w;
                arbWAVrecord<DEXED_ARBWAV_SIG>* dx;
                arbWAVrecord<AudioSynthWavetable::instrument_data>* t;
              };

/*
 * Value types:
 * i : integer
 * f : float
 * l : float = 2^ n
 * c : choice
 * r : reciprocal = min / n, n=1..max
 * s : string (e.g. path to waveform or wavetable)
 * w : arbitrary waveform
 * t : wavetable instrument
 */
class ParamEntry 
{
  public:
    constexpr ParamEntry(const char* const n, char c) : ValType(c), label(n), min{.i = 0}, max{.i = 0}, xoff(0)  {}
    constexpr ParamEntry(const char* const n,   int l,   int u, char c = 'i') : ValType(c), label(n), min{.i = l}, max{.i = u}, xoff(0)  {}
    constexpr ParamEntry(const char* const n, float l, float u, char c = 'f') : ValType(c), label(n), min{.f = l}, max{.f = u}, xoff(0)  {}
    constexpr ParamEntry(const char* const n, float l, float u, int xo) : ValType('f'), label(n), min{.f = l}, max{.f = u}, xoff(xo)  {}
    constexpr ParamEntry(const char* const n, float l,   int u) : ValType('r'), label(n), min{.f = l}, max{.i = u}, xoff(0)  {}
    constexpr ParamEntry(const char* const n, const ParamChoice *pc,  int u) : ValType('c'), label(n), min{.i = 0}, max{.i = u}, choices(pc), xoff(0)  {}
    constexpr ParamEntry(const char* const n) : ValType('s'), label(n), min{.i = 0}, max{.i = 0}, xoff(0)  {}
    const char ValType;
    const char* const label;    
    const ValUnion min,max;
    const ParamChoice* const choices = nullptr;
    const int16_t xoff;
};
#define PARAM_ENTRY_CHOICES(choices) choices,COUNT_OF(choices)-1

class ParamValue
{
  public:
    ParamValue(  int v) : value{.i = v},valueEndX(-1) {}
    ParamValue(float v) : value{.f = v},valueEndX(-1) {}
    ParamValue(char* v) : value{.s = v},valueEndX(-1) {}
    ParamValue(arbWAVrecord<int16_t>* v) : value{.w = v},valueEndX(-1) {}
    ParamValue(arbWAVrecord<DEXED_ARBWAV_SIG>* v) : value{.dx = v},valueEndX(-1) {}
    ParamValue(arbWAVrecord<AudioSynthWavetable::instrument_data>* v) : value{.t = v},valueEndX(-1) {}
    ValUnion value{0};
    int16_t labelEndX, labelEndY, valueEndX;
};

class ParamPage
{
  public:
    int8_t start; // first parameter to show on this page
    int8_t count; // number of parameters to show on this page
};

#endif // !defined(_PARAMENTRY_H_)
