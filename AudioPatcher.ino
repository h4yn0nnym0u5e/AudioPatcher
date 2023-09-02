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

#include "TeensyDebug.h"

//#define COUNT_OF(a) (sizeof a / sizeof a[0])

#if !defined(SAFE_RELEASE_MANY) || !defined(DYNMIXER_H_)
#error Make sure you have dynamic cores and Audio library!
#endif // !defined(SAFE_RELEASE_MANY) || !defined(DYNMIXER_H_)


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
//  {new AudioObjInstance{objList[AUDIO_EFFECT_DELAY_ID],5,5}},
//  {new AudioObjInstance{objList[AUDIO_MIXER4_ID],110,5}},
//  {new AudioObjInstance{objList[AUDIO_FILTER_LADDER_ID],110,55}},
  {new AudioObjInstance{objList[AUDIO_EFFECT_CHORUS_ID],165+55,110}},
  };
std::vector<PatchcordInstance_t*> cordVec; 

/********************************************************************************************************/
void dumpObjVec(void)
{
  for (size_t i=0;i<objVec.size();i++)
    Serial.printf("%d: %s @ %08X\n",i,objVec.at(i).p->objP->name,(uint32_t) objVec.at(i).p);
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

  while (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD not available!");
    delay(500);
  }
  Serial.println("SD initialised");
  

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
  
  AudioObjInstancePtr aoi = {new AudioObjInstance(objList[AUDIO_SYNTH_WAVEFORM_ID],110,110)};
  //AudioObjInstancePtr aoi2 = {new AudioObjInstance(objList[AUDIO_SYNTH_NOISE_WHITE_ID],165,55)};
  objVec.insert(std::next(objVec.begin(),2),aoi);
  //objVec.insert(std::next(objVec.begin(),3),aoi2);
  //objVec.insert(std::next(objVec.begin(),3),{new AudioObjInstance(objList[AUDIO_ANALYZE_FFT1024_ID],220,55)});
  
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

  for (int i=5;i<4;i++)
  {
    PatchcordInstance_t* pci = new PatchcordInstance_t(objVec.at(6).p,i,objVec.at(7).p,i);
    cordVec.push_back(pci);
  }

  std::stable_sort(objVec.begin(),objVec.end());
  dumpObjVec();
  
  // make some real connections  
  /*
  cordVec.push_back(new PatchcordInstance_t{objVec.at(8).p,0,&theOutputI2S,0}); 
  cordVec.push_back(new PatchcordInstance_t(objVec.at(8).p,0,&theOutputI2S,1));
  cordVec.push_back(new PatchcordInstance_t(objVec.at(5).p,0,objVec.at(8).p,0));
*/
  cordVec.push_back(new PatchcordInstance_t{objVec.at(3).p,0,&theOutputI2S,0}); 
  cordVec.push_back(new PatchcordInstance_t(objVec.at(3).p,0,&theOutputI2S,1));
  cordVec.push_back(new PatchcordInstance_t(objVec.at(2).p,0,objVec.at(3).p,0));
  
  for (auto cord : cordVec)
    display.DrawPatchcord(cord);

  Serial.printf("SGTL5000 at %08x\n",(uint32_t) theControlSGTL5000.streamP.ControlSGTL5000);
  
  theControlSGTL5000.streamP.ControlSGTL5000->setAddress(HIGH);
  theControlSGTL5000.streamP.ControlSGTL5000->enable();
  theControlSGTL5000.streamP.ControlSGTL5000->volume(0.1);
 
  delay(5);
  halt_cpu();

  next = millis() + 50;
  systemState = 6;
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
ParamEditor paramEditor(enc0,display,objVec);
DeleteEditor deleteEditor(enc0,enc1,enc2,display,objVec,cordVec);
FileEditor fileEditor(enc0,enc1,enc2,display,objVec,cordVec);
/********************************************************************************************************/
// "Lock" the mode encoder, e.g. while sub-editor is active
// Allows re-use followed by restoration of old state
LimitedEncoderStash* modeEncoderStash;
bool modeEncoderLocked;

void lockModeEncoder(void)
{
  if (!modeEncoderLocked)
  {
    modeEncoderStash = new LimitedEncoderStash(encM);
    modeEncoderLocked = true;
  }
}


void unlockModeEncoder(void)
{
  if (modeEncoderLocked)
  {
    delete modeEncoderStash;
    modeEncoderStash = nullptr;
    modeEncoderLocked = false;
  }
}

//======================================================================
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40)
extern uint8_t _heap_start,_heap_end;
#define HEAP_START &_heap_start
#define HEAP_END   &_heap_end
#else
extern uint8_t __bss_end;
#define HEAP_START &__bss_end
#define HEAP_END   (&_estack - 4096) // guess - allow 4k stack
#endif

void getHeap(int& usedHeap, int& freeHeap)
{
  char* a = (char*) malloc(10000);
  int u = ((uint32_t) a) - ((uint32_t) HEAP_START); // heap used
  int f = ((uint32_t) HEAP_END) - ((uint32_t) a);   // heap free
  //Serial.printf("Heap used %lu bytes; free %lu bytes\n",u,f);  
  free(a);
  usedHeap = u; freeHeap = f;
}

void updateStatus(void)
{
  static elapsedMillis next;
  if (next > 999)
  {
    next = 0;

    float cpu = AudioProcessorUsageMax();
    char buffer[15];
    AudioProcessorUsageMaxReset();
    sprintf(buffer,cpu>9.99f?"CPU:%.1f%%":"CPU:%.2f%%",cpu);
    display.ShowStatus(buffer,320-9*6,0,0xD01C);

    int usedHeap, freeHeap;
    getHeap(usedHeap, freeHeap);
    sprintf(buffer,"Free:%3dk",freeHeap / 1024);
    display.ShowStatus(buffer,320-9*6,1,0xD01C);    
  }
}

//======================================================================
void loop() 
{
  if (!initialised)
  {
    enc2.setValue(enc2.getValue()); // ensures it's valid!
    fileEditor.loadLast();
  }
    
  // Change mode of operation
  if (encM.available() || !initialised)
  {
    if (!modeEncoderLocked)
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
          case 'E': paramEditor.exit(); break;
          case 'D': deleteEditor.exit(); break;
          case 'F': fileEditor.exit(); break;
        }
        switch (editMode[0])
        {
          default: break;
          case 'O': objEditor.enter(); break;  
          case 'P': cordEditor.enter(); break;
          case 'E': paramEditor.enter(); break;
          case 'D': deleteEditor.enter(); break;
          case 'F': fileEditor.enter(); break;
        }
      }
    }
  }

  switch (editMode[0])
  {
    default: break;
    case 'O': objEditor.edit(); break;
    case 'P': cordEditor.edit(); break;    
    case 'E': paramEditor.edit(); break;
    case 'D': deleteEditor.edit(); break;
    case 'F': fileEditor.edit(); break;
  }

  if (!initialised)
    initialised = true;

  updateStatus();    
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
