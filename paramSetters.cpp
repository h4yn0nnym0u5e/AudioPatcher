#include "objects.h"
#include "display.h"


//===========================================================================================
// Strong definitions of setup controls
struct contextChorus {short* mem; int voices;};
int editChorus(AudioObjInstance* aoi, AudioEditMode mode)
{
  contextChorus* myContext = (contextChorus*) aoi->context;
  AudioEffectChorus* me = aoi->streamP.Chorus;
  int modeI = (int) mode;
  static uint32_t exitAt;
  int result = 0;

  if (0 == modeI || -1 == modeI)
  {
    Serial.printf("It's a %s object! %s! At %08X; systemState = %d @ %08X\n",
                  aoi->objP->name,
                  modeI==-1?"Destroyed":"Constructed",
                  (uint32_t) aoi,
                  systemState,(uint32_t) &display
                  ); 
  }
  
  switch ((AudioEditMode) mode)
  {
    case AudioEditMode::constructor: // construction
      {
        size_t memsize = 0.05f * AUDIO_SAMPLE_RATE * sizeof(short); // the man say 50ms - do it
        myContext = new contextChorus;
        aoi->context = myContext;
        myContext->mem = (short*) malloc(memsize);
        myContext->voices = 3;
        me->begin(myContext->mem,memsize,myContext->voices);
      }
      break;
      
    case AudioEditMode::destructor: // destruction
      free(myContext->mem);
      delete myContext;
      break;

    case AudioEditMode::enter:
      exitAt = millis() + 1000; // claim for 1s
      result = 1; // claimed
      break;      

    case AudioEditMode::edit:
      result = exitAt > millis();
      break;      

    default:
      break;      
  }
  return result;  
}
