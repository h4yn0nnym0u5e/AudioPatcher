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

//#define COUNT_OF(a) (sizeof a / sizeof a[0])

/********************************************************************************************************/
M5w_8angle    ctrl;
M5w_8encoder  encr;
uint32_t next;

extern AudioObjStatic_t objList[];
#define COUNT_OF_objList ((int) (AUDIO_MAX_ID - 1))
std::vector<AudioObjStatic_t> objVec = {objList[1],objList[2],objList[5],objList[10]};
 
/********************************************************************************************************/
void setup() {
  display.Init();
  display.Splash();
  while (!Serial)
    ;
  display.Clear();   

  Serial.println("Setup");
  Serial.printf("%d audio objects available\n",AUDIO_MAX_ID - 1);

  int xp = 2,yp = 2;
  for (int i=1;i<COUNT_OF_objList;i++)
  {
    Serial.printf("%32.32s: ID=%d, %d inputs, %d outputs\n",
                  objList[i].name, i, objList[i].inputs, objList[i].outputs);

/*                  
    display.DrawAudioObject(objList[i],xp,yp);
    xp += 48;
    if (xp > 320 - 48)
    {
      xp = 0;
      yp += 48;
    }
*/
  }

/*
  display.DrawConnection(objList[3], 2+2*48, 2+0*48, 0,false,ILI9341_ORANGE);
  display.DrawConnection(objList[3], 2+2*48, 2+0*48, 4,true,0xFC02);
*/
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

  //auto it = objVec.begin();
  //objVec.insert(std::next(it,2),objList[20]);
  objVec.insert(std::next(objVec.begin(),2),objList[20]);
  for (auto obj : objVec)
    Serial.printf("%s\n",obj.name);

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
LimitedEncoder enc0{encr,0,0,31}; // x position in steps of 10
LimitedEncoder enc1{encr,1,0,23}; // y position in steps of 10
LimitedEncoder enc2{encr,2,1,COUNT_OF_objList}; // object selector
void loop() 
{
  if (enc2.available())
  {
    int v = enc2.getValue();
    Serial.printf("%d : %s\n",v,objList[v].name);
    display.ShowSelection(objList[v].name,objList[v].category);
  }

  if (enc0.available() || enc1.available())
  {
    int16_t x = enc0.getValue() * 10;
    int16_t y = enc1.getValue() * 10;
    display.CursorTo(x,y);
  }

  if (encr.getButton(0))
    enc0.setValue(0);
}

/********************************************************************************************************/
extern "C" {
  void startup_late_hookX(void)
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
