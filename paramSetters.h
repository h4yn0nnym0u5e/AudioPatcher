#if !defined(_PARAMSETTERS_H_)
#define _PARAMSETTERS_H_

#include "paramEntry.h"
#include "display.h"
#include "M5wire.h"

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

//==========================================================================
extern ParamEditor* pe;
extern M5w_8angle    ctrl;
extern M5w_8encoder  encr;
extern int testExit(uint32_t& exitAt);


#define BOX_DEF(width,lines) (320/2 - (width)/2),(240/2 - (27+(lines)*16+16)/2),(width),(27+(lines)*16+16)

//=====================================================================================
template <class Tctxt>
void updateFromControls(Tctxt* myContext, AudioObjInstance* aoi)
{
  for (size_t i=0; i < Tctxt::paramCount; i++)
  {
    if (Scale(myContext->params[i],myContext->aray[i],ctrl.getPot16(i),0.999f))
    {
      pe->display.ShowValue(myContext->params[i],myContext->aray[i],i);
      myContext->setParam(i,aoi);
    }
  }
}


//=====================================================================================
template <class Tctxt>
void enterEditMode(Tctxt* myContext, AudioObjInstance* aoi){} // no special action for most AudioStream classes
//=====================================================================================
template <class Tctxt>
void exitEditMode(Tctxt* myContext, AudioObjInstance* aoi){} // no special action for most AudioStream classes
//=====================================================================================
template <class Tstream, class Tctxt>
int editObjType(AudioObjInstance* aoi, AudioEditMode mode, void* params)
{
  Tctxt* myContext = (Tctxt*) aoi->context;
  //Tstream* me = (Tstream*) aoi->streamP.streamObj;
  static uint32_t exitAt;
  static uint32_t next;
  int result = 0;

  switch (mode)
  {
    case AudioEditMode::constructor: // construction
      {        
        myContext = new Tctxt;
        aoi->context = myContext;
        for (int i=0; i < Tctxt::paramCount; i++)
          myContext->setParam(i,aoi); // send initial settings to AudioStream object
      }
      break;
      
    case AudioEditMode::destructor:
      delete myContext;
      break;

    case AudioEditMode::enter:
      result = 1; // claimed
      enterEditMode(myContext,aoi);
      pe = new ParamEditor(display,BOX_DEF(Tctxt::boxWidth,Tctxt::paramCount));
      pe->display.ShowTitle(aoi->objP->name,5,5);
      for (size_t i=0; i < Tctxt::paramCount; i++)
      {
        pe->display.ShowLabel(myContext->params[i],myContext->aray[i],i,5,27);
        pe->display.ShowValue(myContext->params[i],myContext->aray[i],i);
        if (!ctrl.isHooking(i))
          HookControl(ctrl,i,myContext->params[i],myContext->aray[i]);
      }
      next = 0;
      break;      

    case AudioEditMode::edit:
      if (millis() >= next)
      {
        next = millis() + 10;
        updateFromControls(myContext,aoi);
      }
      result = testExit(exitAt);
      break;      

    case AudioEditMode::exit:
      exitEditMode(myContext,aoi);
      for (size_t i=0; i < Tctxt::paramCount; i++)
        ctrl.clearHook(i);
      delete pe;
      pe = nullptr;
      break;  
          
    case AudioEditMode::getParams:
      {
        getSetParams* p = (getSetParams*) params;
        size_t left = p->sz;
        char* ptr = p->buffer;
        int off = 0;
        for (size_t i=0; i < Tctxt::paramCount && off >= 0 && left >= 10; i++)
        {
          switch (myContext->params[i].ValType)
          {
            case 'l':
            case 'f': off = sprintf(ptr,"%f,",myContext->aray[i].value.f); break;
            case 'c':
            case 'i': off = sprintf(ptr,"%d,",myContext->aray[i].value.i); break;
          }

          ptr += off;
          left -= off;            
        }
        p->sz = ptr - p->buffer;
        result = 1;
      }
      break;

    // patch L: ~2: 4,8.774944,0.226114,0.294576,0.000000,      
    case AudioEditMode::setParams:
      {
        getSetParams* p = (getSetParams*) params;
        char* ptr = p->buffer;
        int off = 0;
        for (size_t i=0; i < Tctxt::paramCount && off >= 0; i++)
        {
          ValUnion value;
          
          switch (myContext->params[i].ValType)
          {
            case 'f':
            case 'l':
              sscanf(ptr,"%f,%n",&value.f,&off);
              if (value.f < myContext->params[i].min.f || value.f > myContext->params[i].max.f)
                value.f = (myContext->params[i].min.f + myContext->params[i].max.f) / 2.0f;
              Serial.printf("%.3f ... ",value.f);
              myContext->aray[i].value.f = value.f;
              break;
              
            case 'i':
            case 'c':
              sscanf(ptr,"%d,%n",&value.i,&off);
              if (value.i < myContext->params[i].min.i || value.i > myContext->params[i].max.i)
                value.i = (myContext->params[i].min.i + myContext->params[i].max.i) / 2.0f;
              Serial.printf("%d ... ",value.i);
              myContext->aray[i].value.i = value.i;
              break;
          }
          myContext->setParam(i,aoi);

          ptr += off;
        }
        p->sz = ptr - p->buffer;
        result = 1;
      }
      break;
      
    default:
      break;      
  }
  return result;  
}

//==========================================================================

#endif // !defined(_PARAMSETTERS_H_)
