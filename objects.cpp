#include "objects.h"

#define AUDIO_ENTRY(a,b,c,d,e,f,g,...) INIT_OBJ_STATIC_DATA(a,b,c,d,e,f,g)
AudioObjStatic_t objList[] =
{
  {0},
  AUDIO_ENTRIES
};
#undef AUDIO_ENTRY

AudioObjInstance::AudioObjInstance(AudioObjStatic_t& o, int16_t _x, int16_t _y) 
  : objP(&o), x(_x),y(_y), inputUsedFlags(0) 
{
  switch (objP->id)
  {
#define AUDIO_ENTRY(typ,shrt,id,x,y,cls,label,cons) case id##_ID: streamP.shrt = new typ(cons); break;
    AUDIO_ENTRIES
#undef AUDIO_ENTRY 
    default:
      streamP.Bitcrusher = nullptr; // pick any type, really
      break;     
  }
}


AudioObjInstance::~AudioObjInstance() 
{
  switch (objP->id)
  {
#define AUDIO_ENTRY(typ,shrt,id,x,y,cls,label,cons) case id##_ID: delete streamP.shrt; break;
    AUDIO_ENTRIES
#undef AUDIO_ENTRY 
    default:
      break;     
  }
}

bool operator<(const AudioObjInstancePtr& lhs, const AudioObjInstancePtr& rhs)
{
  return lhs.p->x*10000 + lhs.p->y < rhs.p->x*10000 + rhs.p->y;
}
