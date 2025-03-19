#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)

extern void makeFFP(char* buf, const char* base, const char* path, const char* leaf, const char* ext);

extern const int16_t arbWAV_sax[];

template<typename Tdata>
class arbWAVrecord
{
    static const int ARB_WAV_SAMPLES = 256;
  public:
    arbWAVrecord() :
      sampleData{nullptr}, path{nullptr}, recSize{0}, loaded{false}
      {}
    Tdata* sampleData; // actual 256-sample data block or wavetable instrument
    char* path;        // where it was loaded from
    int index;         // index in sinstrument file (SF2 only)
    size_t recSize;
    bool loaded;

    // Load arbitrary waveform using complete file path
    bool load(const char* buf);

    // Load arbitrary waveform using separate path elements
    bool load(const char* base, const char* path, const char* nme, const char* extn)
    {
      char buf[100];

      makeFFP(buf,base,path,nme,extn);
      return load(buf);
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


    char* prepare(size_t pathLen);
    void reset(void); // reset to default waveform
    void setAll(Tdata* s, char* p, size_t sz, bool l, int i = -1)
    {
      sampleData = s; 
      path = p;
      index = i;
      recSize = sz;
      loaded = l;
    }
    bool isDefault(void);
};

template<> void arbWAVrecord<int16_t>::reset(void);
template<> char* arbWAVrecord<int16_t>::prepare(size_t);
template<> void arbWAVrecord<AudioSynthWavetable::instrument_data>::reset(void);
template<> char* arbWAVrecord<AudioSynthWavetable::instrument_data>::prepare(size_t);


class ParamChoice
{
  public:
    const char* const text;
    const int value;
};

union ValUnion {int i; float f; char* s; arbWAVrecord<int16_t>* w;};

/*
 * Value types:
 * i : integer
 * f : float
 * l : float = 2^ n
 * c : choice
 * r : reciprocal = min / n, n=1..max
 * s : string (e.g. path to waveform or wavetable)
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
