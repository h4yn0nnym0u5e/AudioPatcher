#include "objects.h"

#define AUDIO_ENTRY(a,b,c,d,e,f,g,...) INIT_OBJ_STATIC_DATA(a,b,c,d,e,f,g)
AudioObjStatic_t objList[] =
{
  {0},
  AUDIO_ENTRIES
};
#undef AUDIO_ENTRY
