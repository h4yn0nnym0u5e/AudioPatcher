#if !defined(_PARAMENTRY_H_)
#define _PARAMENTRY_H_

class ParamChoice
{
  public:
    const char* text;
    const int value;
};

union ValUnion {int i; float f;};

class ParamEntry 
{
  public:
    ParamEntry(const char* n,   int l,   int u) : type(0), label(n), min{.i = l}, max{.i = u}  {}
    ParamEntry(const char* n, float l, float u) : type(1), label(n), min{.f = l}, max{.f = u}  {}
    ParamEntry(const char* n, ParamChoice *pc,  int u) : type(2), label(n), min{.i = 0}, max{.i = u}, choices(pc)  {}
    int type;
    const char* label;    
    ValUnion min,max;
    ParamChoice* choices = nullptr;
};
#define PARAM_ENTRY_CHOICES(choices) choices,COUNT_OF(choices)-1

class ParamValue
{
  public:
    ParamValue(  int v) : value{.i = v} {}
    ParamValue(float v) : value{.f = v} {}
    ValUnion value;
    int16_t labelEndX, labelEndY, valueEndX;
};

#endif // !defined(_PARAMENTRY_H_)
