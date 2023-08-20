#include "objects.h"
#include "display.h"


//===========================================================================================
// Strong definitions of setup controls
struct contextChorus {short* mem; int voices;};
int editChorus(AudioObjInstance* aoi, int mode)
{
  contextChorus* myContext = (contextChorus*) aoi->context;
  AudioEffectChorus* me = aoi->streamP.Chorus;
  
    Serial.printf("It's a %s object! %s! At %08X; systemState = %d @ %08X\n",
                  aoi->objP->name,
                  mode==-1?"Destroyed":"Constructed",
                  (uint32_t) aoi,
                  systemState,(uint32_t) &display
                  ); 
    switch (mode)
    {
      case 0: // construction
        {
          size_t memsize = 0.05f * AUDIO_SAMPLE_RATE * sizeof(short); // the man say 50ms - do it
          myContext = new contextChorus;
          aoi->context = myContext;
          myContext->mem = (short*) malloc(memsize);
          myContext->voices = 3;
          me->begin(myContext->mem,memsize,myContext->voices);
        }
        break;
        
      case -1: // destruction
        free(myContext->mem);
        delete myContext;
        break;
    }
    return 0;  
}
