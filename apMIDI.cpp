#include "apMIDI.h"

static USBHost myUSB;
static MIDIDevice midi1(myUSB);
static PatcherMIDI* pm;

static std::vector<PatcherVoice*> sounding;
static std::vector<PatcherVoice*> releasing;
//=================================================================
static const float notesFromC0orMIDI_12[] 
  {
    16.3515978312875f, // C
    17.3239144360545f, // C#
    18.354047994838f,  // D
    19.4454364826301f, // Eb
    20.6017223070544f, // E
    21.8267644645628f, // F
    23.1246514194772f, // F#
    24.4997147488594f, // G
    25.9565435987466f, // Ab
    27.5f,             // A
    29.1352350948807f, // Bb
    30.8677063285078f  // B
  };

static float noteToFreq(byte note)
{
  float result = notesFromC0orMIDI_12[note % 12];

  if (note >= 12)
    result *= 1 << ((note - 12) / 12);
  else
    result /= 2.0f;

  return result;    
}

//=================================================================
void myNoteOn(byte channel, byte note, byte velocity)
{
  float freq = noteToFreq(note);
  
  for (auto obj : pm->objVec)
  {
    switch (obj.p->objP->id) 
    {
      default:
        break;

      case AUDIO_SYNTH_WAVEFORM_ID:
        Serial.printf("Note on: %d -> %.3f\n",note,freq);
        obj.p->streamP.Waveform->frequency(freq);
        obj.p->streamP.Waveform->amplitude(velocity / 127.0f);
        break;        
        
      case AUDIO_EFFECT_ENVELOPE_ID:
        obj.p->streamP.Envelope->noteOn();
        break;
    }
  }
}

void myNoteOff(byte channel, byte note, byte velocity)
{
  for (auto obj : pm->objVec)
  {
    switch (obj.p->objP->id) 
    {
      default:
        break;

      case AUDIO_SYNTH_WAVEFORM_ID:
        Serial.printf("Note off\n");
        //obj.p->streamP.Waveform->amplitude(0.0f);
        break;        
        
      case AUDIO_EFFECT_ENVELOPE_ID:
        obj.p->streamP.Envelope->noteOff();
        break;
    }
  }
  
}


//=================================================================
void PatcherMIDI::init(void)
{
  pm = this;
  myUSB.begin();

  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandleNoteOff(myNoteOff);
}

void PatcherMIDI::update(void)
{
  midi1.read();
}

//=================================================================
PatcherVoice::PatcherVoice(std::vector<AudioObjInstancePtr>& objVec,
                           std::vector<PatchcordInstance_t*>& cordVec)
{
  // clone the per-voice objects
  for (auto obj : objVec)
    if (obj.p->perVoice)
    {
      AudioObjInstance* aoi = new AudioObjInstance(*obj.p->objP, obj.p->x, obj.p->y); 
      voiceVec.push_back({aoi});
      obj.p->copySettingsTo(*aoi);
    }

  // clone the patchcords
  for (auto cord : cordVec)
  {
    AudioObjInstance* src = nullptr, *dst = nullptr;
    int dstPort = -1;

    // look for internal connections
    for (auto obj : voiceVec)
    {
      if (obj.p->isCopyOf(*cord->src))
        src = obj.p;
      if (obj.p->isCopyOf(*cord->dst))
      {
        dst = obj.p;
        dstPort = cord->dst_port;
      }

      // can exit early if both connections found
      if (nullptr != src && nullptr != dst)
        break;
    }

    // cord doesn't connect to any per-voice 
    // object, so it's of no interest to us
    if (nullptr == src && nullptr == dst)
      continue;

    // if we don't have a source, we must have a destination, so
    // the destination is a clone and the source is the
    // same as for the overall design, e.g. an LFO
    if (nullptr == src)
      src = cord->src;

    // if we don't have an internal destination, we're 
    // looking for a mixer with an unused input: this
    // is the only legal option
    if (nullptr == dst)
    {
      if (AudioCategory_mixer == cord->dst->objP->category
       && 0 != cord->dst->inputAvailFlags)
      {
        dst = cord->dst;

        dstPort = 0;
        for (uint32_t avail = cord->dst->inputAvailFlags; 
             0 == (avail & 1);
             avail >>= 1)
          dstPort++;
      }
    }

    // can we clone this cord? Do it!
    if (nullptr != src && nullptr != dst && dstPort >= 0)
    {
      voiceCordVec.push_back(new PatchcordInstance_t(src,cord->src_port,dst,dstPort)); 
    }
  }
}

PatcherVoice::~PatcherVoice()
{
    // delete our patchcords: go backwards
    // so we don't change the index of cords we
    // haven't checked yet
    for (int i=voiceCordVec.size() - 1;i>=0;i--)
    {
      auto cord = voiceCordVec.at(i);
      delete cord;
      voiceCordVec.erase(std::next(voiceCordVec.begin(),i));
    }  

    // similarly for the cloned objects
    for (int i=voiceVec.size() - 1;i>=0;i--)
    {
      auto obj = voiceVec.at(i);
      delete obj.p;
      voiceVec.erase(std::next(voiceVec.begin(),i));
    }  
}

void PatcherVoice::noteOn(byte channel, byte note, byte velocity){}
void PatcherVoice::noteOff(byte channel, byte note, byte velocity){}
