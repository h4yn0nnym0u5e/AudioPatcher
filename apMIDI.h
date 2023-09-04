#if !defined(_AP_MIDI_H_)
#define _AP_MIDI_H_

#include <USBHost_t36.h>

#include "objects.h"
#include <vector> 

class PatcherMIDI
{
  public:
    PatcherMIDI(std::vector<AudioObjInstancePtr>& o)
      : objVec(o)
      {}
    std::vector<AudioObjInstancePtr>& objVec;
    void init(void);
    void update(void);      
};
#endif // !defined(_AP_MIDI_H_)
