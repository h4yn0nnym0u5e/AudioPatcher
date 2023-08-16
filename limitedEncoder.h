#include <M5Wire.h>

class LimitedEncoder
{
    M5w_8encoder& enc;
    uint8_t channel;
    int32_t lower,upper,value, valuex2; 
  public:
    LimitedEncoder(M5w_8encoder& e,uint8_t c,int32_t l,int32_t u)
      : enc(e),channel(c), lower(l),upper(u), value(0),valuex2(0)
      {}
    bool available(void);
    int32_t getValue(void) { return value; }
    void setValue(int32_t v);
    uint8_t getButton(void) { return enc.getButton(channel); }
    void setLimits(int32_t l,int32_t u);
};
