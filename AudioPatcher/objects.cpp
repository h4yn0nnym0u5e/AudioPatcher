#include "objects.h"
#include "display.h"

//===========================================================================================
void editTogglePerVoice(AudioObjInstance* aoi)
{
  if (!aoi->noDelete) // make sure it's a deletable object
  {
    aoi->perVoice = !aoi->perVoice;
    display.DrawPerVoice(*aoi);
  }
}

//===========================================================================================
// Weak definitions of setup controls
extern "C" { 
  int editDoNothing(AudioObjInstance* aoi, AudioEditMode mode, void* params) 
  { 
    if (AudioEditMode::MIDIenter == mode)
    {
      editTogglePerVoice(aoi);
      //Serial.printf("Toggle\n");
    }

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
AudioObjInstance::AudioObjInstance(AudioObjStatic_t& o, int16_t _x, int16_t _y, 
  bool _noD, 
  AudioObjInstance* original)
  : objP(&o), streamP{nullptr}, context(nullptr), x(_x),y(_y), inputAvailFlags(0), 
  noDelete(_noD), perVoice(false), 
  isAcopy(nullptr == original), 
  drawInGrey(false) 
{
  // set all inputs (0..N-1) as available
  if (0 != objP->inputs)
  {
    inputAvailFlags  = 1<<(objP->inputs-1);
    inputAvailFlags |= inputAvailFlags-1;
  }
  
  switch (objP->id)
  {

#define xpAUDIO_ENTRY(typ,shrt,id,x,y,cls,label,...) case id##_ID: \
    Serial.printf("new %s; constructor (%s)", #shrt, #__VA_ARGS__); Serial.flush(); \
    /* streamP.shrt = new typ(__VA_ARGS__); */ edit##shrt(this,AudioEditMode::constructor, original); break;
#define xxAUDIO_ENTRY(typ,shrt,id,x,y,cls,label,...) case id##_ID: \
    /* streamP.shrt = new typ(__VA_ARGS__); */ edit##shrt(this,AudioEditMode::constructor, original); break;
#define AUDIO_ENTRY(typ,shrt,id,x,y,cls,label,...) xxAUDIO_ENTRY(typ,shrt,id,x,y,cls,label,__VA_ARGS__) 

    AUDIO_ENTRIES
    MY_AUDIO_IO

#undef AUDIO_ENTRY 
#undef xxAUDIO_ENTRY 
#undef xpAUDIO_ENTRY 

    default:
      streamP.streamObj = nullptr;
      break;     
  }
  //Serial.printf("Created %s at %08X\n",o.name,(uint32_t) streamP.streamObj);
}


AudioObjInstance::~AudioObjInstance() 
{
  //Serial.printf("Destroyed %s at %08X\n",objP->name,(uint32_t) streamP.streamObj);
  if (nullptr != streamP.streamObj)
  {
    switch (objP->id)
    {
#define AUDIO_ENTRY(typ,shrt,id,x,y,cls,label,...) case id##_ID: delete streamP.shrt; edit##shrt(this,AudioEditMode::destructor, nullptr); break;
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
  return (lhs.p->x*10000 + lhs.p->y) < (rhs.p->x*10000 + rhs.p->y);
}


void AudioObjInstance::copySettingsTo(AudioObjInstance& aoi)
{
  aoi.x = x;
  aoi.y = y;
  aoi.noDelete = noDelete;
  aoi.perVoice = perVoice;
  // don't copy available inputs, that's not a setting as such

  // Copy the parameter settings to the AudioStream objects
  // This may briefly set e.g. the wrong frequency / amplitude
  // for a Waveform, but we have to assume an envelope
  CopyContext(context,aoi.context);
  editSetStreamParams(aoi);
}


bool AudioObjInstance::isCopyOf(AudioObjInstance& aoi)
{
  return    x == aoi.x      // same ...
      &&    y == aoi.y      // ...place, and...
      && objP == aoi.objP;  // ...same type
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
