/*
 * Test of M5 8Angle
 * 8x pot, 9x RGB, 1x switch
 */

#include "M5wire.h"
#include "limitedEncoder.h"
#include "objects.h"
#include "display.h"
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
#define COUNT_OF_objList ((int) (AUDIO_MAX_ID - 1))
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

//======================================================================
void editObject(void)
{
  // Scroll through available object types
  if (enc2.available() || !initialised)
  {
    int v = enc2.getValue();
    Serial.printf("%d : %s\n",v,objList[v].name);
    display.ShowSelection(objList[v].name,objList[v].category);
  }

  // Place an instance of the currently-selected object type
  if (enc2.getButton())
    state = 1;
  else
  {
    if (1 == state) // enc2 released
    {
      int16_t x,y;
      
      state = 0;
      display.GetCursor(x,y);
      objVec.push_back({new AudioObjInstance(objList[enc2.getValue()],x-24,y-24)});
                               
      AudioObjInstance* ao = objVec.back().p;;
      display.CursorClear();
      display.DrawAudioObject(*ao->objP,ao->x,ao->y);
      display.CursorRestore();
      std::stable_sort(objVec.begin(),objVec.end());
    }
  }

  // Move the cross-hair cursor
  if (enc0.available() || enc1.available() || !initialised)
  {
    int16_t x = enc0.getValue() * 10;
    int16_t y = enc1.getValue() * 10;
    display.CursorTo(x,y);
  } 
}

//======================================================================
void highlightObjnum(int n, uint16_t colour)
{
  if (!objVec.empty() && n < (int) objVec.size())
  {
    auto it = objVec.at(n).p;
    display.HighlightAudioObject(it->x,it->y,colour);
  }  
}

//======================================================================
void highlightPort(AudioObjInstance* aoi, int io, int n, bool on)
{
  uint16_t colour = on?PATCHCORD_COLOUR:CONNECTION_COLOUR;
  display.DrawConnection(*aoi->objP,aoi->x,aoi->y,n,0 == io,colour);
}

//======================================================================
int epIdx,portNum;
PatchcordInstance_t editCord;
void editPatchcord(void)
{
  bool changedIdx = false;
  int io = 1-enc2.getValue(); // i = 1 = input = dst, o = 0 = output = src
  
  // select an audio object
  if (enc0.available())
  {
    int ec1 = enc0.getValue();
    AudioObjInstance* aoi = objVec.at(epIdx).p;

    if (aoi != editCord.src && aoi != editCord.dst)
    {
      highlightObjnum(epIdx,ILI9341_BLACK);
      highlightPort(aoi,io,portNum,false);
    }
    else
      highlightObjnum(epIdx,PATCHCORD_COLOUR);
    
    if (ec1 >= 0 && ec1 < (int) objVec.size())
      epIdx = ec1;
    else
      enc0.setValue(epIdx);
    highlightObjnum(epIdx,ILI9341_WHITE);
    
    changedIdx = true;
  }

  // select a connection port
  if (changedIdx || enc1.available())
  {
    int newPortNum = enc1.getValue();
    AudioObjInstance* aoi = objVec.at(epIdx).p;
    int numPorts = io?aoi->objP->inputs:aoi->objP->outputs;

    highlightPort(aoi,io,portNum,false);
    if (0 == numPorts)
    {
      io = 1-io;
      enc2.setValue(1-io);
      numPorts = io?aoi->objP->inputs:aoi->objP->outputs;
      changedIdx = true;
    }
    
    if (newPortNum >= numPorts)
    {
      newPortNum = numPorts - 1; // always >= 0
    }

    portNum = newPortNum;
    enc1.setValue(portNum);
    highlightPort(aoi,io,portNum,true);
  }
  
  if (changedIdx || enc2.available()) // force an update
  {
    char buf[20];
    AudioObjInstance* aoi = objVec.at(epIdx).p;
    highlightPort(aoi,io,portNum,false);
    
    io = 1-enc2.getValue();
    sprintf(buf,"%s",io?"dst":"src");
    display.ShowSelection(buf,AudioCategory_patchcord);
  }

  if (encr.getButton(3))
  {
    if (1 == io) // set dst / input
    {
      if (editCord.dst == objVec.at(epIdx).p)
        editCord.dst = nullptr;        
      else
      {
        editCord.dst = objVec.at(epIdx).p;
        editCord.dst_port = portNum;
      }      
    }
    else // set src / output
    {
      if (editCord.src == objVec.at(epIdx).p)
        editCord.src = nullptr;        
      else
      {
        editCord.src = objVec.at(epIdx).p;
        editCord.src_port = portNum;      
      }
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
    editMode[0] = modes[encM.getValue()];
    display.ShowMode(editMode);
    switch (editMode[0])
    {
      default:
        break;

      case 'O':
        enc0.setLimits(0,31);
        enc2.setLimits(1,COUNT_OF_objList);
        break;

      case 'P':
        enc0.setValue(epIdx);
        enc0.setLimits(0,objVec.size() -1);
        enc2.setValue(0);
        enc2.setLimits(0,1);
        break;
    }
  }

  switch (editMode[0])
  {
    case 'O':
      editObject();
      break;
      
    case 'P':
      editPatchcord();
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
