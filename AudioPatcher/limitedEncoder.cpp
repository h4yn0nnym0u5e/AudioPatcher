#include"limitedEncoder.h"

PROGMEM uint32_t LimitedEncoder::colours[9] =
{ // L B  G R
  0x6400'002A, // red
  0x6400'0A2A, // orange
  0x6400'1515, // yellow
  0x6400'2A00, // green
  0x6415'2000, // cyan
  0x642A'0000, // blue
  0x6460'0015, // purple
  0x6415'1515, // white
  0x6405'0505  // dim white
  /*
  0x2100'0080, // red
  0x2100'2080, // orange
  0x2100'4040, // yellow
  0x2100'8000, // green
  0x2140'6000, // cyan
  0x2180'0000, // blue
  0x2160'0040, // purple
  0x2140'4040, // white
  0x2110'1010  // dim white
  */
};

bool LimitedEncoder::available(int stepBy)
{
  bool result = false;
  int32_t dv = enc.getIncrement(channel);
  int32_t oldValue = value;

  if (dv != 0 || valueSet)
  {
    // deal with injected encoder value
    result = valueSet;
    valueSet = false;
    
    //Serial.printf("dv=%d: ",dv);
    if (dv != (int32_t) 0xDEADBEEF)
    {
      dv *= stepBy;
      
      valuex2 += dv;
      if (value != valuex2 / 2)
      {
        value = valuex2 / 2;
        if (value < lower)
        {
          value = lower;
          valuex2 = value*2;
        }
        if (value > upper)
        {
          value = upper;
          valuex2 = value*2;
        }
        result = value != oldValue;
      }
    }
  }
  
  return result;
}

void LimitedEncoder::setValue(int32_t v, bool makeAvailable /* = false */) 
{ 
  value = v; 
  
  if (value < lower)
    value = lower;
  if (value > upper)
    value = upper;
    
  valuex2 = (value*2 | (valuex2 & 1)); 

  valueSet = makeAvailable;
}

void LimitedEncoder::setLimits(const int32_t l, const int32_t u)
{
  lower = l;
  upper = u;

  setValue(value); // ensure value is in limits
}

void LimitedEncoder::getLimits(int32_t& l, int32_t& u)
{
  l = lower;
  u = upper;
}
