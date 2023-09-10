#if !defined(_AP_MIDI_H_)
#define _AP_MIDI_H_

#include <USBHost_t36.h>
#include <MIDI.h>

#include "objects.h"
#include <vector> 

struct MIDIevent
{
  byte channel,type;
  union {byte data1; byte note;};
  union {byte data2; byte velocity;};
};

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
    // vectors of items copied from the design for this voice
    std::vector<AudioObjInstancePtr> voiceVec;
    std::vector<PatchcordInstance_t*> voiceCordVec;
    int triggerNote;
    int triggerVelocity;
    bool patchOK;

  public:
    PatcherVoice(std::vector<AudioObjInstancePtr>& objVec,
                 std::vector<PatchcordInstance_t*>& cordVec);
    ~PatcherVoice();

    static const float notesFromC0orMIDI_12[12];
    static float noteToFreq(byte note, const float* table = notesFromC0orMIDI_12);
    void noteOn(byte channel, byte note, byte velocity);
    void noteOff(byte channel, byte note, byte velocity);
    void sendMIDIevent(MIDIevent& m);
    bool isActive(void);
    bool isOK(void) { return patchOK; }
    int getNote(void) { return triggerNote; }
};

extern const float notesHammond[];
#endif // !defined(_AP_MIDI_H_)
