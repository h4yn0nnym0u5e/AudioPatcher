#if !defined(_LIMITEDENCODER_H_)
#define _LIMITEDENCODER_H_

#include <M5wire.h>

class LimitedEncoder
{
    M5w_8encoder& enc;
    uint8_t channel;
    int32_t lower,upper,value, valuex2;
    bool valueSet; // flag if value set and should appear as new reading
    uint8_t wasPressed;
    uint32_t LEDcolour;
    static uint32_t colours[9];
  public:
    LimitedEncoder(M5w_8encoder& e,uint8_t c,int32_t l,int32_t u)
      : enc(e),channel(c), lower(l),upper(u), 
        value(0),valuex2(0), valueSet(false), wasPressed(0)
      {}
    bool available(int stepBy = 1);
    int32_t getValue(void) { return value; }
    void setValue(int32_t v, bool makeAvailable = false);
    uint8_t getButton(void) { return enc.getButton(channel); }
    uint8_t wasClicked(void)
    {
      uint8_t result=0, clickNow = enc.getButton(channel);

      if (!clickNow)
        result = wasPressed;
      wasPressed = clickNow;
      
      return result;
    }
    void setLimits(const int32_t l, const int32_t u);
    void getLimits(int32_t& l,int32_t& u);
    void setLED(uint32_t colour)	//!< colour: <brightness><B><G><R>
        { LEDcolour = colour; enc.writeLED(channel,colour); }
    uint32_t getColour(void) { return LEDcolour; }        
    static uint32_t colour(unsigned int n) { if (n>8) n=8; return colours[n];}        
};

class LimitedEncoderStash
{
    int32_t v,l,u;
    LimitedEncoder& e;
  public:
    LimitedEncoderStash(LimitedEncoder& _e) 
      : e(_e)
      { e.available(); v = e.getValue(); e.getLimits(l,u); 
      Serial.printf("Stashed %08X: %d / %d / %d\n",(uint32_t) &e,l,v,u);
      }
    ~LimitedEncoderStash() {e.available(); e.setLimits(l,u); e.setValue(v); 
    Serial.printf("Un-stashed %08X: %d / %d / %d\n",(uint32_t) &e,l,v,u);
    }
};
#endif // !defined(_LIMITEDENCODER_H_)
