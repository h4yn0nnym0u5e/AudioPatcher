#if !defined(_PARAMSETTERS_H_)
#define _PARAMSETTERS_H_

#include "paramEntry.h"
#include "display.h"

class ParamEditor 
{
  public:
    ParamEditor(AudioPatcherDisplay& d, 
    int16_t x, int16_t y, int16_t w, int16_t h) 
    : display(d),
      workArea{.x=x, .y=y, .w=w, .h=h}
    {
      display.SaveArea(x,y,w,h);
      display.InitArea(x,y,w,h);
    }
    ~ParamEditor() { display.RestoreArea(); }
    
    AudioPatcherDisplay& display;
    struct {int16_t x,y,w,h;} workArea;
};
#endif // !defined(_PARAMSETTERS_H_)
