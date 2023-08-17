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

extern AudioObjStatic_t objList[];
std::vector<AudioObjInstancePtr> objVec = {
  {new AudioObjInstance{objList[AUDIO_EFFECT_DELAY_ID],5,5}},
  {new AudioObjInstance{objList[AUDIO_MIXER4_ID],110,5}},
  {new AudioObjInstance{objList[AUDIO_FILTER_LADDER_ID],110,55}},
  {new AudioObjInstance{objList[AUDIO_EFFECT_CHORUS_ID],55,55}},
  };
std::vector<PatchcordInstance_t*> cordVec; 
/********************************************************************************************************/
void setup() 
{
  AudioMemory(50);
  
  display.Init();
  display.Splash();
  while (!Serial)
    ;
  display.Clear();   

  Serial.println("Setup");
  Serial.printf("%d audio objects available\n",AUDIO_MAX_ID - 1);

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
  
  AudioObjInstancePtr aoi = {new AudioObjInstance(objList[AUDIO_SYNTH_SIMPLE_DRUM_ID],110,110)};
  AudioObjInstancePtr aoi2 = {new AudioObjInstance(objList[AUDIO_SYNTH_NOISE_WHITE_ID],165,55)};
  objVec.insert(std::next(objVec.begin(),2),aoi);
  objVec.insert(std::next(objVec.begin(),3),aoi2);
  objVec.insert(std::next(objVec.begin(),3),{new AudioObjInstance(objList[AUDIO_ANALYZE_FFT1024_ID],220,55)});
  
  for (auto obj : objVec)
  {
    Serial.printf("%s\n",obj.p->objP->name);
    display.DrawAudioObject(*obj.p->objP,obj.p->x,obj.p->y);
  }

  for (int i=0;i<4;i++)
  {
    PatchcordInstance_t* pci = new PatchcordInstance_t(objVec.at(0).p,i,objVec.at(1).p,i);
    //*pci = {new AudioConnection(),objVec.at(0).p,i,objVec.at(1).p,i)};
    cordVec.push_back(pci);
  }

  for (auto cord : cordVec)
    display.DrawPatchcord(cord);

  std::stable_sort(objVec.begin(),objVec.end());
  delay(5);

  next = millis() + 50;
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
const char* modes = "OPED"; // objects, patchcords, editing, deleting
LimitedEncoder encM{encr,0,0,(int32_t) strlen(modes)-1}; // mode
LimitedEncoder enc0{encr,1,0,31}; // x position in steps of 10
LimitedEncoder enc1{encr,2,0,23}; // y position in steps of 10
LimitedEncoder enc2{encr,3,1,COUNT_OF_objList}; // object selector
int state = 0;
bool initialised = false;
char editMode[2] = {0};
ObjEditor objEditor(enc0,enc1,enc2,display,objVec,objList);
CordEditor cordEditor(enc0,enc1,enc2,display,objVec,objList);


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
      }
      switch (editMode[0])
      {
        default: break;
        case 'O': objEditor.enter(); break;  
        case 'P': cordEditor.enter(); break;
      }
    }
  }

  switch (editMode[0])
  {
    case 'O':
      objEditor.edit();
      break;
      
    case 'P':
      cordEditor.edit();
      break;
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

  if (!initialised)
    initialised = true;
}

/********************************************************************************************************/
extern "C" {
  void startup_late_hook(void)
  {
    while (!Serial)
      ;
    Serial.println("Late hook");
    if (CrashReport)
    {
      Serial.print(CrashReport);
    }
  }
}
