#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

union ValUnion {int i; float f;};

class ParamEntry 
{
  public:
    ParamEntry(const char* n,   int l,   int u) : type(0), label(n), min{.i = l}, max{.i = u}  {}
    ParamEntry(const char* n, float l, float u) : type(1), label(n), min{.f = l}, max{.f = u}  {}
    int type;
    const char* label;    
    ValUnion min,max;
};

class ParamValue
{
  public:
    ParamValue(  int v) : value{.i = v} {}
    ParamValue(float v) : value{.f = v} {}
    ValUnion value;
    int16_t labelEndX, labelEndY, valueEndX;
};
#endif // !defined(_PARAMENTRY_H_)
