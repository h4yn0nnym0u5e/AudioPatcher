/*
 * Test of M5 8Angle
 * 8x pot, 9x RGB, 1x switch
 */

#include "M5wire.h"
#include "limitedEncoder.h"
#include "objects.h"
#include "display.h"
#include "editors.h"
#include <iterator>
#include <vector>
#include <algorithm>

//#include "TeensyDebug.h"

//#define COUNT_OF(a) (sizeof a / sizeof a[0])

/********************************************************************************************************/
M5w_8angle    ctrl;
M5w_8encoder  encr;
uint32_t next;
int systemState;

extern AudioObjStatic_t objList[];

// Non-deletable objects: I/O and control
#define AUDIO_ENTRY(typ,shrt,id, ...) AudioObjInstance the##shrt{objList[id##_ID],-1,-1,true};
MY_AUDIO_IO
#undef AUDIO_ENTRY

std::vector<AudioObjInstancePtr> objVec = {
#define AUDIO_ENTRY(typ,shrt,id, ...) {&the##shrt},
  MY_AUDIO_IO
#undef AUDIO_ENTRY
  {new AudioObjInstance{objList[AUDIO_EFFECT_DELAY_ID],5,5}},
  {new AudioObjInstance{objList[AUDIO_MIXER4_ID],110,5}},
  {new AudioObjInstance{objList[AUDIO_FILTER_LADDER_ID],110,55}},
  {new AudioObjInstance{objList[AUDIO_EFFECT_CHORUS_ID],165,110}},
  };
std::vector<PatchcordInstance_t*> cordVec; 



std::vector<AudioObjInstancePtr> ioVec = {
  };

/********************************************************************************************************/
void dumpObjVec(void)
{
  for (size_t i=0;i<objVec.size();i++)
    Serial.printf("%d: %s @ %08X\n",i,objVec.at(i).p->objP->name,(uint32_t) objVec.at(i).p);
}

AudioSynthWaveformModulated* pWav;
AudioSynthWaveformModulated* findTheWav(void)
{
  AudioSynthWaveformModulated* result = nullptr;
  
  for (size_t i=0;i<objVec.size();i++)
  {
    auto obj = objVec.at(i);
    if (obj.p->objP->id == AUDIO_SYNTH_WAVEFORM_MODULATED_ID)
    {
      //Serial.printf("Waveform is at[%d]\n",i);
      result = obj.p->streamP.WaveformModulated;
      //pWav->begin(0.5,220.0f,WAVEFORM_SINE);
      break;
    }
  }
  return result;
}
/********************************************************************************************************/
void setup() 
{
  systemState = 4;
  AudioMemory(50);
  
  display.Init();
  display.Splash();
  while (!Serial)
    ;
  display.Clear();   
  systemState = 5;

  Serial.println("Setup");
  Serial.printf("%d audio object types available\n",AUDIO_MAX_ID - 2);

  for (int i=1;i<COUNT_OF_objList;i++)
  {
    Serial.printf("%32.32s: ID=%d, %d inputs, %d outputs\n",
                  objList[i].name, i, objList[i].inputs, objList[i].outputs);
  }

  ctrl.begin();
  Serial.printf("8Angle says %d\n",ctrl?1:0);
  Serial.printf("8Angle at address 0x%02X; version %d\n",ctrl.getAddress(),ctrl.getVersion());
  ctrl.writeLED(0,0xFF00'0080);
  ctrl.writeLED(1,0xFF00'2070);
  ctrl.writeLED(2,0xFF00'4040);
  ctrl.writeLED(3,0xFF00'8000);
  ctrl.writeLED(4,0xFF40'6000);
  ctrl.writeLED(5,0xFF80'0000);
  ctrl.writeLED(6,0xFF60'0040);
  ctrl.writeLED(7,0xFF40'4040);
  ctrl.writeLED(8,0xFF10'1010);

  encr.begin();
  Serial.printf("8Encoder says %d\n",encr?1:0);
  Serial.printf("8Encoder at address 0x%02X; version %d\n",encr.getAddress(),encr.getVersion());

  //halt_cpu();
  
  AudioObjInstancePtr aoi = {new AudioObjInstance(objList[AUDIO_SYNTH_WAVEFORM_MODULATED_ID],55,110)};
  AudioObjInstancePtr aoi2 = {new AudioObjInstance(objList[AUDIO_SYNTH_NOISE_WHITE_ID],165,55)};
  objVec.insert(std::next(objVec.begin(),2),aoi);
  objVec.insert(std::next(objVec.begin(),3),aoi2);
  objVec.insert(std::next(objVec.begin(),3),{new AudioObjInstance(objList[AUDIO_ANALYZE_FFT1024_ID],220,55)});
  
  theInputI2S.x = -40;
  theInputI2S.y = 100;
  theOutputI2S.x = 320 - 8;
  theOutputI2S.y = 100;
  theControlSGTL5000.x = 1;
  theControlSGTL5000.y = 240 - 48 - 20 - 2;
  
  for (auto obj : objVec)
  {
    Serial.printf("%s\n",obj.p->objP->name);
    display.DrawAudioObject(*obj.p->objP,obj.p->x,obj.p->y);
  }

  for (auto obj : ioVec)
  {
    Serial.printf("%s\n",obj.p->objP->name);
    display.DrawAudioObject(*obj.p->objP,obj.p->x,obj.p->y);
  }

  for (int i=0;i<4;i++)
  {
    PatchcordInstance_t* pci = new PatchcordInstance_t(objVec.at(6).p,i,objVec.at(7).p,i);
    //*pci = {new AudioConnection(),objVec.at(0).p,i,objVec.at(1).p,i)};
    cordVec.push_back(pci);
  }

  std::stable_sort(objVec.begin(),objVec.end());
  dumpObjVec();
  
  // make some real connections  
  cordVec.push_back(new PatchcordInstance_t{objVec.at(7).p,0,&theOutputI2S,0}); //cordVec.back()->connect();
  cordVec.push_back(new PatchcordInstance_t(objVec.at(7).p,0,&theOutputI2S,1));
  cordVec.push_back(new PatchcordInstance_t(objVec.at(3).p,0,objVec.at(7).p,0));

  for (auto cord : cordVec)
    display.DrawPatchcord(cord);

  Serial.printf("SGTL5000 at %08x\n",(uint32_t) theControlSGTL5000.streamP.ControlSGTL5000);
  
  theControlSGTL5000.streamP.ControlSGTL5000->setAddress(HIGH);
  theControlSGTL5000.streamP.ControlSGTL5000->enable();
  theControlSGTL5000.streamP.ControlSGTL5000->volume(0.1);

  
  pWav = findTheWav();
  if (nullptr != pWav)
  {
    Serial.printf("Waveform is at 0x%08X\n",(uint32_t) pWav);
    pWav->begin(0.5,220.0f,WAVEFORM_SINE);
  }
  
  delay(5);

  next = millis() + 50;
  systemState = 6;
}

/********************************************************************************************************/
void hookup(int ch1, int ch2)
{
  ctrl.setHook(ch1,ctrl.getLast(ch2));
}

int id = 1,id2 = 2;
void xloop() 
{
  id2 += encr.getIncrement(0);
  if (millis() > next || id != id2/2)
  {
    next = millis() + 250;
    id = id2 / 2;
    if (id >= COUNT_OF_objList)
    {
      id = COUNT_OF_objList - 1;
      id2 = id*2;
    }
    if (id < 1)
    {
      id = 1;
      id2 = id*2;
    }
    Serial.printf("%d %d; %d %d %s\n",ctrl.getPot16(0),ctrl.getPot16(1),id2,id,objList[id].name);
  }

  if (encr.getButton(0))
    hookup(0,1);

  if (encr.getButton(1))
    hookup(1,0);

  delay(1);
}

/********************************************************************************************************/
const char* modes = "OPEDF"; // objects, patchcords, editing, deleting, filing
LimitedEncoder encM{encr,0,0,(int32_t) strlen(modes)-1}; // mode
LimitedEncoder enc0{encr,1,0,31}; // x position in steps of 10
LimitedEncoder enc1{encr,2,0,23}; // y position in steps of 10
LimitedEncoder enc2{encr,3,1,COUNT_OF_objList}; // object selector
int state = 0;
bool initialised = false;
char editMode[2] = {0};
ObjEditor objEditor(enc0,enc1,enc2,display,objVec,objList);
CordEditor cordEditor(enc0,enc1,enc2,display,objVec,cordVec,objList);
DeleteEditor deleteEditor(enc0,enc1,enc2,display,objVec,cordVec);
FileEditor fileEditor(enc0,enc1,enc2,display,objVec,ioVec,cordVec);

//======================================================================
void wavControl(void)
{
  static uint32_t next;
  if (millis() >= next)
  {
    next = millis() + 5;
    pWav = findTheWav();
    if (nullptr != pWav)
    {
      pWav->amplitude((float) ctrl.getPot16(0) / 4096.0f);
      pWav->frequency((float) ctrl.getPot16(1)*2.0f + 10.0f);
    }
  }
}

//======================================================================
void loop() 
{
  if (!initialised)
  {
    enc2.setValue(enc2.getValue()); // ensures it's valid!
  }
    
  // Change mode of operation
  if (encM.available() || !initialised)
  {
    char oldMode = editMode[0];
    editMode[0] = modes[encM.getValue()];

    if (oldMode != editMode[0])
    {
      display.ShowMode(editMode);
      switch (oldMode)
      {
        default: break;
        case 'O': objEditor.exit(); break;  
        case 'P': cordEditor.exit(); break;
        case 'D': deleteEditor.exit(); break;
        case 'F': fileEditor.exit(); break;
      }
      switch (editMode[0])
      {
        default: break;
        case 'O': objEditor.enter(); break;  
        case 'P': cordEditor.enter(); break;
        case 'D': deleteEditor.enter(); break;
        case 'F': fileEditor.enter(); break;
      }
    }
  }

  switch (editMode[0])
  {
    default: break;
    case 'O': objEditor.edit(); break;
    case 'P': cordEditor.edit(); break;
    
    case 'D': deleteEditor.edit(); break;
    case 'F': fileEditor.edit(); break;
  }

  
  if (encr.getButton(0))
    enc0.setValue(0);


  // Test highlighting
  if (encr.getButton(6))
  {
      int16_t x,y;
      
      display.GetCursor(x,y);
      display.HighlightAudioObject(x-24,y-24);
  }   
  if (encr.getButton(7))
  {
      int16_t x,y;
      
      display.GetCursor(x,y);
      display.HighlightAudioObject(x-24,y-24,false);
  }   

  wavControl();

  if (!initialised)
    initialised = true;
}

/********************************************************************************************************/
extern "C" {
  void startup_late_hook(void)
  {
    systemState = 1;
    while (!Serial)
      ;
    systemState = 1;
    Serial.println("Late hook");
    if (CrashReport)
    {
      Serial.print(CrashReport);
    }
    systemState = 3;
  }
}
