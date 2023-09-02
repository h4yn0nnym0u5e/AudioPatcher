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
    ParamEntry(const char* n,   int l,   int u) : ValType('i'), label(n), min{.i = l}, max{.i = u}  {}
    ParamEntry(const char* n,   int l,   int u, char c) : ValType(c), label(n), min{.i = l}, max{.i = u}  {}
    ParamEntry(const char* n, float l, float u) : ValType('f'), label(n), min{.f = l}, max{.f = u}  {}
    ParamEntry(const char* n, float l, float u, char c) : ValType(c), label(n), min{.f = l}, max{.f = u}  {}
    ParamEntry(const char* n, const ParamChoice *pc,  int u) : ValType('c'), label(n), min{.i = 0}, max{.i = u}, choices(pc)  {}
    const char ValType;
    const char* label;    
    const ValUnion min,max;
    const ParamChoice* choices = nullptr;
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

#endif // !defined(_PARAMENTRY_H_)
