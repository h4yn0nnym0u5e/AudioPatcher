#include "objects.h"
#include "display.h"

//===========================================================================================
// Weak definitions of setup controls
extern "C" { 
  int editDoNothing(AudioObjInstance* aoi, AudioEditMode mode, void* params) 
  { 
    /*
    if (AudioEditMode::constructor == mode || AudioEditMode::destructor == mode)
    {
      Serial.printf("%s a %s, at %08X; systemState = %d @%08X\n",
                  mode==AudioEditMode::destructor?"Destroyed":"Constructed",
                  aoi->objP->name,
                  (uint32_t) aoi,
                  systemState,(uint32_t) &display
                  ); 
    }
    */
    return 0; 
  }
}
#define AUDIO_ENTRY(typ,shrt,...) int edit##shrt(AudioObjInstance*, AudioEditMode, void*) __attribute__((weak, alias("editDoNothing"))); 
AUDIO_ENTRIES
MY_AUDIO_IO
#undef AUDIO_ENTRY


//===========================================================================================
#define AUDIO_ENTRY(a,b,c,d,e,f,g,...) INIT_OBJ_STATIC_DATA(a,b,c,d,e,f,g)
AudioObjStatic_t objList[] =
{
  {0},
  AUDIO_ENTRIES
  {0}, // space for the AUDIO_MAX_ID guard value
  MY_AUDIO_IO
};
#undef AUDIO_ENTRY

//===========================================================================================
AudioObjInstance::AudioObjInstance(AudioObjStatic_t& o, int16_t _x, int16_t _y, bool _noD) 
  : objP(&o), context(nullptr), x(_x),y(_y), inputAvailFlags(0), noDelete(_noD) 
{
  // set all inputs (0..N-1) as available
  if (0 != objP->inputs)
  {
    inputAvailFlags  = 1<<(objP->inputs-1);
    inputAvailFlags |= inputAvailFlags-1;
  }
  
  switch (objP->id)
  {
#define AUDIO_ENTRY(typ,shrt,id,x,y,cls,label,cons) case id##_ID: streamP.shrt = new typ(cons); edit##shrt(this,AudioEditMode::constructor, nullptr); break;
    AUDIO_ENTRIES
    MY_AUDIO_IO
#undef AUDIO_ENTRY 
    default:
      streamP.streamObj = nullptr;
      break;     
  }
}


AudioObjInstance::~AudioObjInstance() 
{
  if (nullptr != streamP.streamObj)
  {
    switch (objP->id)
    {
#define AUDIO_ENTRY(typ,shrt,id,x,y,cls,label,cons) case id##_ID: edit##shrt(this,AudioEditMode::destructor, nullptr); delete streamP.shrt; break;
      AUDIO_ENTRIES
      MY_AUDIO_IO
#undef AUDIO_ENTRY 
      default:
        break;     
    }
  }
}

bool operator<(const AudioObjInstancePtr& lhs, const AudioObjInstancePtr& rhs)
{
  return lhs.p->x*10000 + lhs.p->y < rhs.p->x*10000 + rhs.p->y;
}

//===========================================================================================
PatchcordInstance_t::~PatchcordInstance_t()
{
  if (nullptr != conn)
    delete conn;

  if (nullptr != dst) // destination port now unused
    dst->inputAvailFlags |= 1<<dst_port; // flag that input is available
}

void PatchcordInstance_t::connect(void)
{      
  if (nullptr != src
   && nullptr != dst
   && nullptr != src->streamP.streamObj
   && nullptr != dst->streamP.streamObj)
  {
    conn->connect(*src->streamP.streamObj,src_port, *dst->streamP.streamObj,dst_port);
    dst->inputAvailFlags &= ~(1<<dst_port); // this input isn't available
  }
}

//===========================================================================================
int objNameToID(const char* s)
{
  int result;
  for (result = COUNT_OF_objTypes; result >= 0; result--)
  {
    if (nullptr != objList[result].name && 0 == strcmp(s,objList[result].name))
      break;
  }
  return result;
}
