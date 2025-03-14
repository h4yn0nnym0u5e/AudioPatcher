#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

#if !defined(COUNT_OF)
#define COUNT_OF(a) (sizeof a / sizeof a[0])
#endif // !defined(COUNT_OF)


extern const int16_t arbWAV_sax[];
class arbWAVrecord
{
    static const int ARB_WAV_SAMPLES = 256;
  public:
    int16_t* sampleData; // actual 256-sample data block
    char* path;          // where it was loaded from
    size_t recSize;
    bool loaded;

    bool load(const char* base, const char* path, const char* nme, const char* extn);
    bool load(const char* buf);
    char* prepare(size_t pathLen);
    void reset(void); // reset to default waveform
    void setAll(int16_t* s, char* p, size_t sz, bool l)
    {
      sampleData = s; 
      path = p;
      recSize = sz;
      loaded = l;
    }
    bool isDefault(void) {return sampleData == arbWAV_sax; }
};

class ParamChoice
{
  public:
    const char* const text;
    const int value;
};

union ValUnion {int i; float f; char* s; arbWAVrecord* w;};

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
    ParamEntry(const char* n, char c) : ValType(c), label(n), min{.i = 0}, max{.i = 0}, xoff(0)  {}
    ParamEntry(const char* n,   int l,   int u, char c = 'i') : ValType(c), label(n), min{.i = l}, max{.i = u}, xoff(0)  {}
    ParamEntry(const char* n, float l, float u, char c = 'f') : ValType(c), label(n), min{.f = l}, max{.f = u}, xoff(0)  {}
    ParamEntry(const char* n, float l, float u, int xo) : ValType('f'), label(n), min{.f = l}, max{.f = u}, xoff(xo)  {}
    ParamEntry(const char* n, float l,   int u) : ValType('r'), label(n), min{.f = l}, max{.i = u}, xoff(0)  {}
    ParamEntry(const char* n, const ParamChoice *pc,  int u) : ValType('c'), label(n), min{.i = 0}, max{.i = u}, choices(pc), xoff(0)  {}
    ParamEntry(const char* n) : ValType('s'), label(n), min{.i = 0}, max{.i = 0}, xoff(0)  {}
    const char ValType;
    const char* label;    
    const ValUnion min,max;
    const ParamChoice* choices = nullptr;
    const int16_t xoff;
};
#define PARAM_ENTRY_CHOICES(choices) choices,COUNT_OF(choices)-1

class ParamValue
{
  public:
    ParamValue(  int v) : value{.i = v},valueEndX(-1) {}
    ParamValue(float v) : value{.f = v},valueEndX(-1) {}
    ParamValue(char* v) : value{.s = v},valueEndX(-1) {}
    ParamValue(arbWAVrecord* v) : value{.w = v},valueEndX(-1) {}
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
