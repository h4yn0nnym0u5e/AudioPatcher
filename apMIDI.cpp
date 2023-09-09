#include "apMIDI.h"

static USBHost myUSB;
static MIDIDevice midi1(myUSB);
static PatcherMIDI* pm;
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
