/*
 * Run-time configurable synthesizer
 * based on Dynamic Audio library
 */

#include "M5wire.h"
#include "limitedEncoder.h"
#include "objects.h"
#include "display.h"
#include "editors.h"
#include "apMIDI.h"
#include <iterator>
#include <vector>
#include <algorithm>

// #include "TeensyDebug.h"

#if !defined(SAFE_RELEASE_MANY) || !defined(DYNMIXER_H_)
#error Make sure you have dynamic cores and Audio library!
#endif // !defined(SAFE_RELEASE_MANY) || !defined(DYNMIXER_H_)

AudioSynthWavetable dummy;

/********************************************************************************************************/
M5w_8angle    ctrl{0x43, Wire1};
M5w_8encoder  encr{0x41, Wire1};
uint32_t next;
int systemState;

extern AudioObjStatic_t objList[];
extern uint8_t external_psram_size;

// Non-deletable objects: I/O and control
#define AUDIO_ENTRY(typ,shrt,id, ...) AudioObjInstance the##shrt{objList[id##_ID],-1,-1,true};
MY_AUDIO_IO
#undef AUDIO_ENTRY

std::vector<AudioObjInstancePtr> objVec = {
#define AUDIO_ENTRY(typ,shrt,id, ...) {&the##shrt},
  MY_AUDIO_IO
#undef AUDIO_ENTRY
  {new AudioObjInstance{objList[AUDIO_EFFECT_CHORUS_ID],165+55-4,110}},
  };
std::vector<PatchcordInstance_t*> cordVec; 

/********************************************************************************************************/
void dumpObjVec(void)
{
  for (size_t i=0;i<objVec.size();i++)
    Serial.printf("%d: %s @ %08X\n",i,objVec.at(i).p->objP->name,(uint32_t) objVec.at(i).p);
}
/********************************************************************************************************/
void printFlashID(void)
{
  uint8_t buf[7] = {0x9F};
  const int pin = 6; // audio board memory
  
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delayMicroseconds(500);
  //SPI.begin();

  SPI.beginTransaction(SPISettings(15'000'000, MSBFIRST, SPI_MODE0));
  digitalWrite(pin, LOW);
  SPI.transfer(buf, sizeof buf);
  SPI.endTransaction();
  digitalWrite(pin, HIGH);
   
  Serial.printf("Audio adaptor memory ID: %02X %02X %02X\n", buf[4], buf[5], buf[6]); 
}
/********************************************************************************************************/
void printHL(void)
{
  Serial.println("============================================================");
}
/********************************************************************************************************/
void doReboot() 
{
  SCB_AIRCR = 0x05FA0004;
}
/********************************************************************************************************/
//
//                    888                      
//                    888                      
//                    888                      
//  .d8888b   .d88b.  888888 888  888 88888b.  
//  88K      d8P  Y8b 888    888  888 888 "88b 
//  "Y8888b. 88888888 888    888  888 888  888 
//       X88 Y8b.     Y88b.  Y88b 888 888 d88P 
//   88888P'  "Y8888   "Y888  "Y88888 88888P"  
//                                    888      
//                                    888      
//                                    888      
//                                             
//
/********************************************************************************************************/
extern const AudioSynthWavetable::instrument_data Harp;
void setup() 
{
  while (!Serial && millis() < 4000)
    ;
  Serial.println("Setup");
  systemState = 4;
  AudioMemory(80);

  printHL();
  display.Init();
  display.Splash();
  display.Clear();   
  systemState = 5;

  dummy.setInstrument(Harp);

  printHL();
  printFlashID();
  Serial.printf("%dMB PSRAM fitted\n", external_psram_size);
  
  while (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD not available!");
    delay(500);
  }
  Serial.println("SD initialised");
  
  Wire1.begin();
  ctrl.begin();
  Serial.printf("8Angle says %d\n",ctrl?1:0);
  Serial.printf("8Angle at address 0x%02X; version %d\n",ctrl.getAddress(),ctrl.getVersion());
  
  ctrl.writeLED(0,0x2F00'0080);
  ctrl.writeLED(1,0x2F00'2070);
  ctrl.writeLED(2,0x2F00'3040);
  ctrl.writeLED(3,0x2F00'8000);
  ctrl.writeLED(4,0x2F40'6000);
  ctrl.writeLED(5,0x2F80'0000);
  ctrl.writeLED(6,0x2F60'0040);
  ctrl.writeLED(7,0x2F40'4040);
  ctrl.writeLED(8,0x2F10'1010);
  /*
  for (int i=0;i<9;i++)
    ctrl.writeLED(i,0);
    */
  encr.begin();
  Serial.printf("8Encoder says %d\n",encr?1:0);
  Serial.printf("8Encoder at address 0x%02X; version %d\n",encr.getAddress(),encr.getVersion());
  
  printHL();
  Serial.printf("%d audio object types available\n",AUDIO_MAX_ID - 2);
  for (int i=1;i<COUNT_OF_objList;i++)
  {
    Serial.printf("%22.22s: ID=%d, %d inputs, %d outputs\n",
                  objList[i].name, i, objList[i].inputs, objList[i].outputs);
  }

  AudioObjInstancePtr aoi = {new AudioObjInstance(objList[AUDIO_SYNTH_WAVEFORM_ID],110,110)};
  objVec.insert(std::next(objVec.begin(),2),aoi);
  
  theInputI2S.x = -40;
  theInputI2S.y = 100;
  theOutputI2S.x = 320 - 8;
  theOutputI2S.y = 100;
  theControlSGTL5000.x = 1;
  theControlSGTL5000.y = 240 - 48 - 20 - 2;
  
  printHL();
  for (auto obj : objVec)
  {
    Serial.printf("%s\n",obj.p->objP->name);
    display.DrawAudioObject(*obj.p);
  }

  for (int i=5;i<4;i++)
  {
    PatchcordInstance_t* pci = new PatchcordInstance_t(objVec.at(6).p,i,objVec.at(7).p,i);
    cordVec.push_back(pci);
  }

  std::stable_sort(objVec.begin(),objVec.end());
  dumpObjVec();
  
  // make some real connections  
  cordVec.push_back(new PatchcordInstance_t{objVec.at(3).p,0,&theOutputI2S,0}); 
  cordVec.push_back(new PatchcordInstance_t(objVec.at(3).p,0,&theOutputI2S,1));
  cordVec.push_back(new PatchcordInstance_t(objVec.at(2).p,0,objVec.at(3).p,0));
  
  for (auto cord : cordVec)
    display.DrawPatchcord(cord);


  printHL();
  Serial.printf("SGTL5000 at %08x\n",(uint32_t) theControlSGTL5000.streamP.ControlSGTL5000);

  {
    uint8_t addr = LOW;
    bool enabled = theControlSGTL5000.streamP.ControlSGTL5000->enable();
    if (!enabled)
    {
      theControlSGTL5000.streamP.ControlSGTL5000->setAddress(HIGH);
      enabled = theControlSGTL5000.streamP.ControlSGTL5000->enable();
    }
    Serial.printf("SGTL5000 init %s at %s address\n",enabled?"success":"failure", addr == LOW?"low":"high");
  }
  theControlSGTL5000.streamP.ControlSGTL5000->volume(0.1);
 
  delay(5);
#if defined(GDB_IS_ENABLED)
  halt_cpu();
#endif // defined(GDB_IS_ENABLED)

  next = millis() + 50;
  systemState = 6;

  printHL();
  Serial.println("loop() starting");
  printHL();
}

/********************************************************************************************************/
//
//           888    d8b 888          
//           888    Y8P 888          
//           888        888          
//  888  888 888888 888 888 .d8888b  
//  888  888 888    888 888 88K      
//  888  888 888    888 888 "Y8888b. 
//  Y88b 888 Y88b.  888 888      X88 
//   "Y88888  "Y888 888 888  88888P' 
//                                   
//
/********************************************************************************************************/
/*
 * Provide an accessible mute function, e.g. for loading a patch
 */
void setMuteStatus(bool mute)
{
  if (mute)
  {
    theControlSGTL5000.streamP.ControlSGTL5000->muteHeadphone();
    theControlSGTL5000.streamP.ControlSGTL5000->muteLineout();
  }
  else
  {
    theControlSGTL5000.streamP.ControlSGTL5000->unmuteHeadphone();
    theControlSGTL5000.streamP.ControlSGTL5000->unmuteLineout();
  }
}


/********************************************************************************************************/
const char* modes = "OPEMDF"; // objects, patchcords, editing, MIDI config, deleting, filing
const char* patchBase = "/patches";
LimitedEncoder encM{encr,0,0,(int32_t) strlen(modes)-1}; // mode
LimitedEncoder enc0{encr,1,0,31}; // x position in steps of 10
LimitedEncoder enc1{encr,2,0,23}; // y position in steps of 10
LimitedEncoder enc2{encr,3,1,COUNT_OF_objList}; // object selector
int state = 0;
bool initialised = false;
char editMode[2] = {0};
ObjEditor objEditor(enc0,enc1,enc2,display,objVec,cordVec,objList);
CordEditor cordEditor(enc0,enc1,enc2,display,objVec,cordVec,objList);
ParamEditor paramEditor(enc0,enc1,enc2,display,objVec,cordVec);
MIDIEditor midiEditor(enc0,display,objVec,cordVec);
DeleteEditor deleteEditor(enc0,enc1,enc2,display,objVec,cordVec);
FileEditor fileEditor(enc0,enc1,enc2,display,objVec,cordVec,patchBase,FileBase::mode_e::del);

PatcherMIDI patcherMIDI(objVec, cordVec);
/********************************************************************************************************/
// "Lock" the mode encoder, e.g. while sub-editor is active
// Allows re-use followed by restoration of old state
// We also set it to a value of 0 and limits of ±1
LimitedEncoderStash* modeEncoderStash;
bool modeEncoderLocked;

void lockModeEncoder(void)
{
  if (!modeEncoderLocked)
  {
    modeEncoderStash = new LimitedEncoderStash(encM);
    modeEncoderLocked = true;
    encM.setLimits(-1,1);
    encM.setValue(0);
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
//
//  888                            
//  888                            
//  888                            
//  888  .d88b.   .d88b.  88888b.  
//  888 d88""88b d88""88b 888 "88b 
//  888 888  888 888  888 888  888 
//  888 Y88..88P Y88..88P 888 d88P 
//  888  "Y88P"   "Y88P"  88888P"  
//                        888      
//                        888      
//                        888      
// 
//======================================================================
void loop() 
{
  //-------------------------------------------------
  // Hacky screen dump code
  static bool b6state = false;
  if (encr.getButton(6))
  {
    b6state = true;
  }
  else
  {
    if (b6state)
    {
      Serial.printf("Current screen dump");
      b6state = false;
      uint16_t* scrbuf;
      size_t sz = 320*240 * sizeof *scrbuf;
      scrbuf = (uint16_t*) malloc(sz);
      if (nullptr != scrbuf)
      {
        // find a non-existent file name
        static int fnum = 0;
        char buf[50];
        do
        {
          sprintf(buf,"/dumps/%05d.bin",fnum);
          if (!SD.exists(buf))
            break;
          fnum++;
        } while (1);
  
        display.SaveAreaToBuffer(0,0,320,240,scrbuf);
        uint32_t sum = 0;
        for (size_t i = 0;i<sz/sizeof *scrbuf;i++)
          sum += scrbuf[i];
        File dmp = SD.open(buf,FILE_WRITE);

        if (dmp)
        {
          dmp.write(scrbuf,sz);
          dmp.close();
          Serial.printf("ed to '%s'; sum = %u\n",buf,sum);
        }
        else
          Serial.println(" failed - no file");
        
        free(scrbuf);
      }
      else
        Serial.println(" failed - no memory");
    }
  }

  //-------------------------------------------------
  // enter debugger, or reset system
  if (encr.getButton(7))
#if defined(GDB_IS_ENABLED)
    halt_cpu();
#else
    doReboot();   
#endif // defined(GDB_IS_ENABLED)

  //-------------------------------------------------
  if (!initialised)
  {
    enc2.setValue(enc2.getValue()); // ensures it's valid!
    fileEditor.loadLast();
    printHL();
    patcherMIDI.init();
    printHL();
  }
    
  //-------------------------------------------------
  // Change mode of operation
  if ((!modeEncoderLocked && encM.available()) || !initialised)
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
        case 'M': midiEditor.exit(); break;
        case 'D': deleteEditor.exit(); break;
        case 'F': fileEditor.exit(); break;
      }
      switch (editMode[0])
      {
        default: break;
        case 'O': objEditor.enter(); break;  
        case 'P': cordEditor.enter(); break;
        case 'E': paramEditor.enter(); break;
        case 'M': midiEditor.enter(); break;
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
    case 'E': paramEditor.edit(); break;
    case 'M': midiEditor.edit(); break;
    case 'D': deleteEditor.edit(); break;
    case 'F': fileEditor.edit(); break;
  }

  if (!initialised)
    initialised = true;

  //-------------------------------------------------
  patcherMIDI.update();
  updateStatus();    
}

/********************************************************************************************************/
extern "C" {
  void startup_late_hook(void)
  {
    systemState = 1;
    while (!Serial && millis() < 4000)
      ;
    systemState = 1;
    Serial.println("Late hook");
    if (CrashReport)
    {
      while (!Serial) // crashed - ensure we see the report
        ;
      Serial.print(CrashReport);
    }
    systemState = 3;
  }
}
