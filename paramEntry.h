#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

class ParamChoice
{
  public:
    const char* const text;
    const int value;
};

union ValUnion {int i; float f;};

class ParamEntry 
{
  public:
    ParamEntry(const char* n,   int l,   int u, char c = 'i') : ValType(c), label(n), min{.i = l}, max{.i = u}, xoff(0)  {}
    ParamEntry(const char* n, float l, float u, char c = 'f') : ValType(c), label(n), min{.f = l}, max{.f = u}, xoff(0)  {}
    ParamEntry(const char* n, float l, float u, int xo) : ValType('f'), label(n), min{.f = l}, max{.f = u}, xoff(xo)  {}
    ParamEntry(const char* n, const ParamChoice *pc,  int u) : ValType('c'), label(n), min{.i = 0}, max{.i = u}, choices(pc), xoff(0)  {}
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
    ValUnion value;
    int16_t labelEndX, labelEndY, valueEndX;
};

class ParamPage
{
  public:
    int8_t start; // first parameter to show on this page
    int8_t count; // number of parameters to show on this page
};

#endif // !defined(_PARAMENTRY_H_)
