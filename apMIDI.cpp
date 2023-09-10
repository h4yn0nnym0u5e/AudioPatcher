#include "apMIDI.h"

static USBHost myUSB;
static MIDIDevice midi1(myUSB);
static PatcherMIDI* pm;

static std::vector<PatcherVoice*> sounding;
static std::vector<PatcherVoice*> releasing;
//=================================================================
const float PatcherVoice::notesFromC0orMIDI_12[] 
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

// Reproduce most of the Hammond tonewheel ratios
static const float HAMMOND_BASE = 1200.0f / 60.0f * 2.0f;
const float notesHammond[] 
  {
    85.0f /104.0f * HAMMOND_BASE, // C
    71.0f /82.0f * HAMMOND_BASE, // C#
    67.0f /73.0f * HAMMOND_BASE, // D
    70.0f /72.0f * HAMMOND_BASE, // D#
    69.0f /67.0f * HAMMOND_BASE, // E
    54.0f /99.0f * HAMMOND_BASE, // F
    37.0f /64.0f * HAMMOND_BASE, // F#
    49.0f /80.0f * HAMMOND_BASE, // G
    48.0f /74.0f * HAMMOND_BASE, // G#
    66.0f /96.0f * HAMMOND_BASE, // A
    67.0f /92.0f * HAMMOND_BASE, // A#
    54.0f /70.0f * HAMMOND_BASE  // B
  };

float PatcherVoice::noteToFreq(byte note, const float* table)
{
  float result = table[note % 12];

  if (note >= 12)
    result *= 1 << ((note - 12) / 12);
  else
    result /= 2.0f;

  return result;    
}

//=================================================================
void myNoteOn(byte channel, byte note, byte velocity)
{

  float freq = PatcherVoice::noteToFreq(note);
  
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

/*
  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandleNoteOff(myNoteOff);
  */
}

void PatcherMIDI::update(void)
{
  // deal with new MIDI events
  if (midi1.read())
  {
    switch (midi1.getType())
    {
      case midi::NoteOn: 
        {
          Serial.println("\nCreate voice:");
          PatcherVoice* newVoice = new PatcherVoice{objVec,cordVec}; // create the voice
          sounding.push_back(newVoice);  // add it to the list
          newVoice->noteOn(midi1.getChannel(),midi1.getData1(), midi1.getData2()); // start it sounding
        }
        break;

      case midi::NoteOff:
        {
          byte note = midi1.getData1();          
        
          for (size_t i=0; i < sounding.size(); i++)
          {
            PatcherVoice* obj = sounding.at(i);  
          
            if (note == obj->getNote())
            {
              obj->noteOff(midi1.getChannel(),note, midi1.getData2());
              releasing.push_back(obj);
              sounding.erase(sounding.begin() + i);
            }
          }
        }
        break; 

      default:
        {
          MIDIevent me {midi1.getChannel(),midi1.getType(),midi1.getData1(),midi1.getData2()};
          for (auto obj : sounding)
            obj->sendMIDIevent(me);
        }
        break;
    }
  }

  // now see if any releasing voices have finished
  for (int i = releasing.size() - 1; i >= 0; i--)
  {
    PatcherVoice* obj = releasing.at(i);
    if (!obj->isActive())
    {
      Serial.println("\nDelete voice:");
      delete obj;
      releasing.erase(releasing.begin() + i);
    }
  }
}

//=================================================================
// Create a new sub-patch from the design. Usually triggered
// by a MIDI note on, but might not be so we don't actually
// do the note on function in here
PatcherVoice::PatcherVoice(std::vector<AudioObjInstancePtr>& objVec,
                           std::vector<PatchcordInstance_t*>& cordVec)
            : triggerNote(-1), triggerVelocity(-1), patchOK(true)
{
  // clone the per-voice objects
  for (auto obj : objVec)
    if (obj.p->perVoice)
    {
      AudioObjInstance* aoi = new AudioObjInstance(*obj.p->objP); 
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
    else
      patchOK = false; // at least one patchcord failure
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
      voiceCordVec.erase(voiceCordVec.begin()+i);
    }  

    // similarly for the cloned objects
    for (int i=voiceVec.size() - 1;i>=0;i--)
    {
      auto obj = voiceVec.at(i);
      delete obj.p;
      voiceVec.erase(voiceVec.begin()+i);
    }  
}

void PatcherVoice::sendMIDIevent(MIDIevent& me)
{
  // send the message to all objects
  for (auto obj : voiceVec)
    obj.p->objP->editFn(obj.p,AudioEditMode::processMIDIevent,&me);  
}


void PatcherVoice::noteOn(byte channel, byte note, byte velocity)
{
  MIDIevent me{channel,midi::NoteOn,{note},{velocity}};
  sendMIDIevent(me);
  triggerNote = note;
  triggerVelocity = velocity;
}
  
void PatcherVoice::noteOff(byte channel, byte note, byte velocity)
{
  MIDIevent me{channel,midi::NoteOff,{note},{velocity}};
  sendMIDIevent(me);
}

bool PatcherVoice::isActive(void)
{
  bool result = false;

  for (auto obj : voiceVec)
    if (0 != obj.p->objP->editFn(obj.p,AudioEditMode::checkIfActive,nullptr)) // found an active object?
    {
      result = true; // still active then, look no further
      break;
    }
  
  return result;
}
