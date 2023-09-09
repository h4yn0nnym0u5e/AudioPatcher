#if !defined(_AP_MIDI_H_)
#define _AP_MIDI_H_

#include <USBHost_t36.h>

#include "objects.h"
#include <vector> 

class PatcherMIDI
{
  public:
    PatcherMIDI(std::vector<AudioObjInstancePtr>& o,
                std::vector<PatchcordInstance_t*>& p )
      : objVec(o), cordVec(p)
      {}
    std::vector<AudioObjInstancePtr>& objVec;
    std::vector<PatchcordInstance_t*>& cordVec;
    void init(void);
    void update(void);      
};

class PatcherVoice
{
  std::vector<AudioObjInstancePtr> voiceVec;
  std::vector<PatchcordInstance_t*> voiceCordVec;
  byte triggerNote;

  public:
    PatcherVoice(std::vector<AudioObjInstancePtr>& objVec,
                 std::vector<PatchcordInstance_t*>& cordVec);
    ~PatcherVoice();
    void noteOn(byte channel, byte note, byte velocity);
    void noteOff(byte channel, byte note, byte velocity);
};
#endif // !defined(_AP_MIDI_H_)
