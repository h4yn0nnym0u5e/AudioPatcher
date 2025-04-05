#include "apMIDI.h"

#define COUNT_OF(a) (int)(sizeof a / sizeof a[0])

/*
 * MIDI host setup.
 */
static USBHost myUSB;
static USBHub hub1{myUSB}; // allow for one hub...
static PatcherMIDI* pm;
static MIDIDevice midiHosts[2] = {{myUSB},{myUSB}}; // ... and two controllers

static std::vector<PatcherVoice*> sounding;
static std::vector<PatcherVoice*> releasing;

//=================================================================
PROGMEM const float velocity2amplitude[128] = {0.00000,
0.01778,0.01966,0.02164,0.02371,0.02588,0.02814,0.03049,0.03294,0.03549,0.03812,
0.04086,0.04369,0.04661,0.04963,0.05274,0.05594,0.05924,0.06264,0.06613,0.06972,
0.07340,0.07717,0.08104,0.08500,0.08906,0.09321,0.09746,0.10180,0.10624,0.11077,
0.11539,0.12011,0.12493,0.12984,0.13484,0.13994,0.14513,0.15042,0.15581,0.16128,
0.16685,0.17252,0.17828,0.18414,0.19009,0.19613,0.20227,0.20851,0.21484,0.22126,
0.22778,0.23439,0.24110,0.24790,0.25480,0.26179,0.26887,0.27605,0.28333,0.29070,
0.29816,0.30572,0.31337,0.32112,0.32896,0.33690,0.34493,0.35306,0.36128,0.36960,
0.37801,0.38651,0.39511,0.40381,0.41260,0.42148,0.43046,0.43953,0.44870,0.45796,
0.46732,0.47677,0.48631,0.49595,0.50569,0.51552,0.52544,0.53546,0.54557,0.55578,
0.56609,0.57648,0.58697,0.59756,0.60824,0.61902,0.62989,0.64085,0.65191,0.66307,
0.67432,0.68566,0.69710,0.70863,0.72026,0.73198,0.74380,0.75571,0.76771,0.77981,
0.79201,0.80430,0.81668,0.82916,0.84174,0.85440,0.86717,0.88003,0.89298,0.90602,
0.91917,0.93240,0.94573,0.95916,0.97268,0.98629,1.00000
};
//=================================================================
PROGMEM const float PatcherVoice::notesFromC0orMIDI_12[] 
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
PROGMEM const float notesHammond[] 
  {
    85.0f /104.0f * HAMMOND_BASE, // C
    71.0f /82.0f * HAMMOND_BASE, // C#
    67.0f /73.0f * HAMMOND_BASE, // D
    105.0f /108.0f * HAMMOND_BASE, // D#
    103.0f /100.0f * HAMMOND_BASE, // E
    84.0f /77.0f * HAMMOND_BASE, // F
    74.0f /64.0f * HAMMOND_BASE, // F#
    98.0f /80.0f * HAMMOND_BASE, // G
    96.0f /74.0f * HAMMOND_BASE, // G#
    88.0f /64.0f * HAMMOND_BASE, // A
    67.0f /46.0f * HAMMOND_BASE, // A#
    108.0f /70.0f * HAMMOND_BASE // B
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
void PatcherMIDI::init(void)
{
  pm = this;
  myUSB.begin();
#if defined(USB_MIDI_SELECTED_IN_TOOLS)
  Serial.print("USB MIDI and ");
#endif
  Serial.println("MIDI host enabled");
}

/*
 * Process a MIDI event received on any of the configured ports,
 * be they USB Sevice, USB Host or Serial.
 * 
 * For note on or off we may create a new instance of all the per-voice
 * objects, and send those the "note on/off" event. Everything else
 * just gets passed to every instantiated object, which may or may
 * not make use of it.
 */
void PatcherMIDI::processEvent(uint8_t cable, uint8_t channel, uint8_t type, 
                               uint8_t data1, uint8_t data2, uint8_t* sysexArray) 
{
    switch (type)
    {
      case midi::NoteOn:       
        {         
          //uint32_t t = micros();
          PatcherVoice* newVoice = new PatcherVoice{objVec, cordVec, *this, designObjectsFree}; // create the voice
          designObjectsFree &= !newVoice->usesDesignObjects(); // flag whether it used the design objects
          sounding.push_back(newVoice);  // add it to the list
          newVoice->noteOn(channel,data1,data2); // start it sounding
          //Serial.printf("Took %uus to instantiate note\n",micros() - t);
        }
        break;

      case midi::NoteOff:
        {
          byte note = data1;
          
          for (size_t i=0; i < sounding.size(); i++)
          {
            PatcherVoice* obj = sounding.at(i);  
          
            if (note == obj->getNote())
            {
              obj->noteOff(channel,note,data2);
              releasing.push_back(obj);
              sounding.erase(sounding.begin() + i);
            }
          }
        }
        break; 

      default:
        {
          byte ch  = channel,
               typ = type,
               d1  = data1,
               d2  = data2;
          uint16_t sysexLength = nullptr == sysexArray?0:((d2<<8)|(d1&0xFF));

          
          // for some messages we want to keep data in
          // case another note is started which needs
          // to make use of it
          switch (typ)
          {
            default:
              break;
              
            case midi::ControlChange:
              Serial.printf("CC %d = %d\n",d1,d2);
              CCvalues[d1] = d2;
              break;

            case midi::PitchBend:
              pitchBend = ((int16_t) d2 << 7) | d1;
              pitchBend -= 0x2000; // make it signed
              break;
          }

          {
            MIDIevent me {ch,typ,d1,d2,sysexLength,sysexArray,DummyVoice};
            PatcherVoice::sendMIDIevent(objVec,me); // send to whole design
          }

          for (auto obj : sounding) // every PatcherVoice that's sounding
          {           
            if (!obj->usesDesignObjects()) // except the one using the design objects
            {
              MIDIevent me {ch,typ,d1,d2,sysexLength,sysexArray,*obj};
              obj->sendMIDIevent(me);
            }
          }
        }
        break;
    }  
}

void PatcherMIDI::update(void)
{
  // deal with new MIDI events
  // usbMIDI is defined by setting USB Type in the Tools menu
#if defined(USB_MIDI_SELECTED_IN_TOOLS)  
  if (usbMIDI.read())
  {
    byte type = usbMIDI.getType();
    uint8_t* syxP = type == midi::SystemExclusive?usbMIDI.getSysExArray():nullptr;
  
    processEvent(usbMIDI.getCable(), usbMIDI.getChannel(), type, usbMIDI.getData1(), usbMIDI.getData2(), syxP);
  }
#endif // defined(USB_MIDI_SELECTED_IN_TOOLS)  

  for (int i=0; i<COUNT_OF(midiHosts); i++)
  {
    MIDIDevice& midiHost = midiHosts[i];

    if (midiHost.read())
    {
      byte type = midiHost.getType();
      uint8_t* syxP = type == midi::SystemExclusive?midiHost.getSysExArray():nullptr;
    
      processEvent(midiHost.getCable(), midiHost.getChannel(), type, midiHost.getData1(), midiHost.getData2(), syxP);
    }
  
  }
  // now see if any releasing voices have finished
  for (int i = releasing.size() - 1; i >= 0; i--)
  {
    PatcherVoice* obj = releasing.at(i);
    if (!obj->isActive())
    {
      //uint32_t t = micros();

      // if voice was using the design objects,
      // it won't be from now on: mark them as free
      if (obj->usesDesignObjects())
        designObjectsFree = true;
      delete obj;
      releasing.erase(releasing.begin() + i);
      //Serial.printf("Took %uus to de-instantiate note\n",micros() - t);
    }
  }
}

//=================================================================
// Create a new sub-patch from the design. Usually triggered
// by a MIDI note on, but might not be so we don't actually
// do the note on function in here
PatcherVoice::PatcherVoice(std::vector<AudioObjInstancePtr>& objVec,
                           std::vector<PatchcordInstance_t*>& cordVec,
                           PatcherMIDI& _pm,
                           bool canUseDesignObjects)
            : PatcherVoiceBase(_pm),
              triggerNote(-1), triggerVelocity(-1), 
              patchOK(true), designObjectsUsed(false)
{
  // clone the per-voice objects, or
  // re-use the design objects
  for (auto obj : objVec)
    if (obj.p->perVoice)
    {
      AudioObjInstance* aoi;
      if (canUseDesignObjects)
        aoi = obj.p;
      else
      {
        aoi = new AudioObjInstance(*obj.p); 
        obj.p->copySettingsTo(*aoi);
      }
      voiceVec.push_back({aoi});
    }

  // clone the patchcords, if necessary
  if (canUseDesignObjects)
  {
    patchOK = true;
    designObjectsUsed = true;    
  }
  else
  {
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
}

PatcherVoice::~PatcherVoice()
{
  if (!designObjectsUsed) // if using design, we didn't create patchcords
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
  }

  // similarly for the cloned objects
  for (int i=voiceVec.size() - 1;i>=0;i--)
  {
    if (!designObjectsUsed) // only delete if we own the objects
    {
      auto obj = voiceVec.at(i);
      delete obj.p;
    }
    voiceVec.erase(voiceVec.begin()+i);
  }  
}

/* static */
void PatcherVoice::sendMIDIevent(std::vector<AudioObjInstancePtr> voiceVec, MIDIevent& me)
{
  // send the message to all objects
  for (auto obj : voiceVec)
    obj.p->objP->editFn(obj.p,AudioEditMode::processMIDIevent,&me);  
}

void PatcherVoice::noteOn(byte channel, byte note, byte velocity)
{
  MIDIevent me{channel,midi::NoteOn,{note},{velocity},0,nullptr,*this};
  sendMIDIevent(me);
  triggerNote = note;
  triggerVelocity = velocity;
}
  
void PatcherVoice::noteOff(byte channel, byte note, byte velocity)
{
  MIDIevent me{channel,midi::NoteOff,{note},{velocity},0,nullptr,*this};
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
