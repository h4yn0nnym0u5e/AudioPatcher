#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

class ParamEntry 
{
  public:
    ParamEntry(const char* n,   int v,   int l,   int u) : type(0), label(n), value{.i = v}, min{.i = l}, max{.i = u}  {}
    ParamEntry(const char* n, float v, float l, float u) : type(1), label(n), value{.f = v}, min{.f = l}, max{.f = u}  {}
    int type;
    const char* label;
    union ParamValue {int i; float f; };
    ParamValue value,min,max;
    int16_t labelEndX, labelEndY, valueEndX;
};

#endif // !defined(_PARAMENTRY_H_)
