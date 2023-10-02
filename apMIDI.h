#if !defined(_AP_MIDI_H_)
#define _AP_MIDI_H_

#include <USBHost_t36.h>
#include <MIDI.h>

#include "objects.h"
#include <vector> 

struct MIDIevent;
class PatcherMIDI;
class PatcherVoiceBase
{
  public:
    PatcherVoiceBase(PatcherMIDI& _pm) : pm(_pm) {}
    PatcherMIDI& pm;  
};

// Each instance of this will serve a single MIDI channel. For
// now, there's just the one.
class PatcherMIDI
{
    byte CCvalues[128]{0}; // actually should be 0-119, but this is safer
    int16_t pitchBend{0}; // 0x2000 is zero: we'll subtract this on reception
    bool designObjectsFree; // true if design objects can be used by PatcherVoice
    PatcherVoiceBase DummyVoice;
    
  public:
    PatcherMIDI(std::vector<AudioObjInstancePtr>& o,
                std::vector<PatchcordInstance_t*>& p )
      : designObjectsFree(true), 
        DummyVoice(*this),
        objVec(o), cordVec(p)
      {}
    std::vector<AudioObjInstancePtr>& objVec;
    std::vector<PatchcordInstance_t*>& cordVec;
    void init(void);
    void update(void);
    byte getCC(byte CCnum) { return CCvalues[CCnum & 0x7F]; }
    int16_t getPitchBend(void) { return pitchBend; }
    float getPitchBend(float sensitivity) { return pow(2,sensitivity*pitchBend/8192/12); }
};

class PatcherVoice : public PatcherVoiceBase
{
    // vectors of items copied from the design for this voice
    std::vector<AudioObjInstancePtr> voiceVec;
    std::vector<PatchcordInstance_t*> voiceCordVec;
    int triggerNote;
    int triggerVelocity;
    bool patchOK;
    bool designObjectsUsed; // true if this voice uses the design objects

  public:
    PatcherVoice(std::vector<AudioObjInstancePtr>& objVec,
                 std::vector<PatchcordInstance_t*>& cordVec,
                 PatcherMIDI& _pm,
                 bool canUseDesignObjects);
    ~PatcherVoice();

    static const float notesFromC0orMIDI_12[12];
    static float noteToFreq(byte note, const float* table = notesFromC0orMIDI_12);

    
    void noteOn(byte channel, byte note, byte velocity);
    void noteOff(byte channel, byte note, byte velocity);
    void sendMIDIevent(MIDIevent& m) { sendMIDIevent(voiceVec,m); }
    static void sendMIDIevent(std::vector<AudioObjInstancePtr> voiceVec, MIDIevent& m);
    bool isActive(void);
    bool isOK(void) { return patchOK; }
    bool usesDesignObjects(void) { return designObjectsUsed; }
    int getNote(void) { return triggerNote; }
    int getVelocity(void) { return triggerVelocity; }
};

struct MIDIevent
{
  byte channel,type;
  union {byte data1; byte note;     byte CCnum; byte PBlsb;};
  union {byte data2; byte velocity; byte CCval; byte PBmsb;};
  PatcherVoiceBase pvb;
};



extern const float notesHammond[];
extern const float velocity2amplitude[128];
#endif // !defined(_AP_MIDI_H_)
