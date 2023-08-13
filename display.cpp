#include "display.h"

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

void displayInit(void)
{
  tft.begin();
  tft.setRotation(TFT_ROTATION);
  tft.setScroll(0);
  displayClear();
}

void displayClear(void)
{
  tft.fillScreen(ILI9341_BLACK);
}


void displaySplash(void)
{
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.print("Here we ");
  delay(500);
  tft.println("go!...");

#define TFT_OBJ_SIZE_X 32
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);
  for (int i=0;i<10;i++)
  {
    tft.fillRoundRect(i*32,100,30,30,4,tft.color565(i*25,0,128));
    tft.drawRoundRect(i*32,100,30,30,4,tft.color565(i*25,i*15+15,128));
    tft.setCursor(i*32+12,118);
    tft.print(i);   
  }
  tft.setCursor(0,105);
  tft.print("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");   
  
  tft.drawLine(0,0,100,100,ILI9341_ORANGE);
  //tft.setScroll(20); // is horizontal, with setRotation(3)
  Serial.println(tft.color565(64,0,128),HEX);
}

struct AudioObjectColours_s {uint16_t body,border,text; }
  AudioObjectColours[] = {{ILI9341_BLACK, ILI9341_WHITE, ILI9341_YELLOW},
                          {ILI9341_NAVY, 0x03FF, ILI9341_WHITE},
                          {0x0280, ILI9341_GREEN, ILI9341_WHITE},
                          {ILI9341_MAROON, ILI9341_RED, ILI9341_WHITE},
                          {0x04200, ILI9341_YELLOW, ILI9341_WHITE}};
struct AudioObjectSizes_t {
  const int ow,oh,cc,cw,ch,ff;                          
} osize = {.ow = 46,.oh = 46, .cc = 4, .cw = 3, .ch = 3, .ff = 6};

void getInputPositions(AudioObjStatic_t& o, int16_t x, int16_t y, 
                       int16_t* ppx, int16_t* ppy, int16_t* pys)
{
  int16_t cb,cs;
  cs = (osize.oh - osize.ff) / o.inputs;
  cb = y + (cs - osize.ch + osize.ff) / 2;

  *ppx = x+1;
  *ppy = cb;
  *pys = cs;
}

void getOutputPositions(AudioObjStatic_t& o, int16_t x, int16_t y, 
                       int16_t* ppx, int16_t* ppy, int16_t* pys)
{
  int16_t cb,cs;
  cs = (osize.oh - osize.ff) / o.outputs;
  cb = y + (cs - osize.ch + osize.ff) / 2;

  *ppx = x+osize.ow-osize.cw-1;
  *ppy = cb;
  *pys = cs;
}

void displayDrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y)
{ 
  const char* label = o.label;
  
  tft.fillRoundRect(x,y,osize.ow,osize.oh,osize.cc,AudioObjectColours[o.category].body);
  tft.drawRoundRect(x,y,osize.ow,osize.oh,osize.cc,AudioObjectColours[o.category].border);
  
  int16_t lw,lh;
  tft.setFontAdafruit();
  tft.setTextSize(1);
  lw = tft.measureTextWidth(o.label);
  lh =  tft.measureTextHeight("Atoy"); // caps, ascenders and descenders
  tft.setTextColor(AudioObjectColours[o.category].text);
  tft.setCursor(x+(osize.ow - lw) / 2,y+osize.oh-lh-2);
  tft.print(o.label);

  int16_t cx,cb,cs;
  if (o.inputs > 0)
  {
    getInputPositions(o,x,y,&cx,&cb,&cs);
    for (int i=0;i<o.inputs;i++)
    {
      tft.fillRect(cx,cb,osize.cw,osize.ch,CONNECTION_COLOUR);
      cb += cs;
    }
  }
  
  if (o.outputs > 0)
  {
    getOutputPositions(o,x,y,&cx,&cb,&cs);
    for (int i=0;i<o.outputs;i++)
    {
      tft.fillRect(cx,cb,osize.cw,osize.ch,CONNECTION_COLOUR);
      cb += cs;
    }
  }
}

void drawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n , bool op, uint16_t colour)
{
#define BAD -999  
  int16_t cx = BAD,cb,cs;
  
  if (op) // drawing an output blob
  {
    if (n < o.outputs)
      getOutputPositions(o,x,y,&cx,&cb,&cs);
  }
  else // input blob
  {
    if (n < o.inputs)
      getInputPositions(o,x,y,&cx,&cb,&cs);
  }
  
  if (BAD != cx) // not impossible - draw it
  {
    cb += cs*n;
    tft.fillRect(cx,cb,osize.cw,osize.ch,colour);
  }
  else
    Serial.println("Bad connection!");
}
