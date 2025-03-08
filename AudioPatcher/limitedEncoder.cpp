#include"limitedEncoder.h"

bool LimitedEncoder::available(void)
{
  bool result = false;
  int32_t dv = enc.getIncrement(channel);

  if (dv != 0)
  {
    //Serial.printf("dv=%d: ",dv);
    if (dv != (int32_t) 0xDEADBEEF)
    {
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
        result = true;
      }
    }
  }
  return result;
}

void LimitedEncoder::setValue(int32_t v) 
{ 
  value = v; 
  
  if (value < lower)
    value = lower;
  if (value > upper)
    value = upper;
    
  valuex2 = (value*2 | (valuex2 & 1)); 
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
