#include "display.h"

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
AudioPatcherDisplay& display = AudioPatcherDisplay::getInstance();
uint16_t behindCursor[100];

void AudioPatcherDisplay::Init(void)
{
  tft.begin();
  tft.setRotation(TFT_ROTATION);
  tft.setScroll(0);
  Clear();
  screenBuffer = (uint16_t*) extmem_malloc(tft.width() * tft.height() * sizeof *screenBuffer);
}

void AudioPatcherDisplay::Clear(void)
{
  tft.fillScreen(ILI9341_BLACK);
}


void AudioPatcherDisplay::SaveArea(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (nullptr != screenBuffer)
  {
    savedArea = {x,y,w,h};
    tft.readRect(x,y,w,h,screenBuffer);
  }
}


void AudioPatcherDisplay::RestoreArea(void)
{
  if (nullptr != screenBuffer)
  {
    tft.writeRect(savedArea.x,savedArea.y,savedArea.h,savedArea.w,screenBuffer);
  }
}


void AudioPatcherDisplay::InitArea(int16_t x, int16_t y, int16_t w, int16_t h)
{
  tft.fillRoundRect(x,y,w,h,6,EDIT_BKGND); 
  tft.fillRoundRect(x,y,w,26,6,ILI9341_BLACK); 
  tft.drawRoundRect(x,y,w,h,6,ILI9341_WHITE); 
  tft.setTextColor(ILI9341_LIGHTGREY,EDIT_BKGND);
  tft.setTextSize(2);
}

void AudioPatcherDisplay::ShowTitle(const char* t, int16_t xoff, int16_t yoff)
{
  tft.setTextColor(0xA514);
  tft.setCursor(savedArea.x + xoff,savedArea.y + yoff); // assume we're operating in the area we saved
  tft.print(t);
  tft.setTextColor(ILI9341_LIGHTGREY,EDIT_BKGND);
}

void AudioPatcherDisplay::ShowLabel(const ParamEntry& p, ParamValue& v, int16_t n, int16_t xoff, int16_t yoff)
{
  int xxoff = xoff + p.xoff;
  tft.setCursor(savedArea.x + xxoff,savedArea.y + n*16 + yoff); // assume we're operating in the area we saved
  tft.print(p.label);
  tft.print(":");
  
  tft.getCursor(&v.labelEndX,&v.labelEndY);
}


void AudioPatcherDisplay::ShowValue(const ParamEntry& p, ParamValue& v, int16_t n)
{
  tft.setCursor(v.labelEndX,v.labelEndY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_LIGHTGREY,EDIT_BKGND);
  switch (p.ValType)
  {
    default: tft.print("???"); break;
    case 'i': tft.print(v.value.i); break;
    case 'f': tft.printf("%.2f",v.value.f); break;
    case 'c': tft.print(p.choices[v.value.i].text); break;
    case 'l': tft.printf("%.2f",pow(2.0f,v.value.f)); break;
  }

  int16_t x,y;
  tft.getCursor(&x,&y);
  if (v.valueEndX > x)
  {
    tft.fillRect(x,v.labelEndY,v.valueEndX - x, 16, EDIT_BKGND);
  }
  v.valueEndX = x;  
}

void AudioPatcherDisplay::Splash(void)
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
  AudioObjectColours[] = {{ILI9341_BLACK, ILI9341_WHITE, ILI9341_YELLOW}, // none
                          {CONNECTION_COLOUR, PATCHCORD_COLOUR, ILI9341_WHITE}, // connection
                          {ILI9341_PURPLE, 0xD01C, ILI9341_WHITE},
                          {ILI9341_NAVY, 0x03FF, ILI9341_WHITE},
                          {0x0280, ILI9341_GREEN, ILI9341_WHITE},
                          {ILI9341_MAROON, ILI9341_RED, ILI9341_WHITE},
                          {0x04200, ILI9341_YELLOW, ILI9341_WHITE}, // synth
                          {ILI9341_BLACK, ILI9341_DARKGREY, ILI9341_WHITE}, // \  ---------------
                          {ILI9341_BLACK, ILI9341_DARKGREY, ILI9341_WHITE}, //  } I/O and control
                          {ILI9341_BLACK, ILI9341_DARKGREY, ILI9341_WHITE}, // /  ---------------
                         };
struct AudioObjectSizes_t {
  const int ow,oh,cc,cw,ch,ff;                          
} osize = {.ow = 46,.oh = 46, .cc = 4, .cw = 3, .ch = 3, .ff = 6};


//=================================================================================================
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


//=================================================================================================
void AudioPatcherDisplay::DrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y, bool greyed)
{ 
  uint16_t bc = greyed?ILI9341_DARKGREY:AudioObjectColours[o.category].body;
  uint16_t ec = greyed?ILI9341_LIGHTGREY:AudioObjectColours[o.category].border;

  tft.fillRoundRect(x,y,osize.ow,osize.oh,osize.cc,bc);
  tft.drawRoundRect(x,y,osize.ow,osize.oh,osize.cc,ec);
  
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

void AudioPatcherDisplay::EraseAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y)
{
  tft.fillRoundRect(x-1,y-1,osize.ow+2,osize.oh+2,osize.cc+1,ILI9341_BLACK);   
}


//=================================================================================================
void AudioPatcherDisplay::HighlightAudioObject(int16_t x, int16_t y, uint16_t colour)
{
  tft.drawRoundRect(x-1,y-1,osize.ow+2,osize.oh+2,osize.cc+1,colour);  
}


void AudioPatcherDisplay::HighlightAudioObject(int16_t x, int16_t y, bool on)
{
  HighlightAudioObject(x,y,(uint16_t)(on?ILI9341_WHITE:ILI9341_BLACK));  
}


//=================================================================================================
void AudioPatcherDisplay::DrawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n , bool op, uint16_t colour)
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
    Serial.printf("%s:%s.%d: bad connection!\n",o.name,op?"output":"input",n);
}


//=================================================================================================
void AudioPatcherDisplay::ShowMode(const char* txt)
{
  uint16_t colour;

  switch (*txt)
  {
    case 'O':
      colour = ILI9341_LIGHTGREY;
      break;

    case 'P':
      colour = PATCHCORD_COLOUR; // orange
      break;

    case 'E':
      colour = ILI9341_GREEN;
      break;

    case 'D':
      colour = 0x721F; // purple-ish
      break;

    case 'F':
      colour = 0xC604; // 
      break;

    default: // ...oops
      colour = ILI9341_PINK;
      break;
  }
  modeColour = colour;
  
  tft.setFontAdafruit();
  tft.setTextSize(2);
  int16_t th = tft.fontLineSpace(); // assume it's going to fit on one line!
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(1,240 - th);

  tft.fillRect(0,239-th,tft.measureTextWidth(txt,1)+1,th+1,colour);
  tft.print(*txt);
}


//=================================================================================================
void AudioPatcherDisplay::ShowBottomText(const char* txt, uint16_t colour)
{
  static int16_t eraseTo = -1;
  bool fixCursor;
  
  tft.setFontAdafruit();
  tft.setTextSize(2);
  int16_t tw = tft.measureTextWidth(txt),
          th = tft.fontLineSpace(); // assume it's going to fit on one line!
  fixCursor = cursor_y > 240-th;
  tft.setTextColor(colour,ILI9341_BLACK);
  tft.setCursor(20,240 - th);
  if (fixCursor)
    CursorClear();
  tft.print(txt);
  if (tw < eraseTo)
    tft.fillRect(20+tw,240 - th,eraseTo - tw, th,ILI9341_BLACK);
  eraseTo = tw;
  if (fixCursor)
    CursorRestore();
}

void AudioPatcherDisplay::ShowSelection(const char* txt, AudioCategory_e cat)
{
  ShowBottomText(txt,AudioObjectColours[cat].border);
}


//=================================================================================================
void AudioPatcherDisplay::ShowStatus(const char* txt, int16_t x, int16_t up, uint16_t colour)
{
  static int16_t eraseTo = -1;
  bool fixCursor;
  
  tft.setFontAdafruit();
  tft.setTextSize(1);
  int16_t tw = tft.measureTextWidth(txt),
          th = tft.fontLineSpace(); // assume it's going to fit on one line!
  fixCursor = cursor_y > 240-th;
  tft.setTextColor(colour,ILI9341_BLACK);
  tft.setCursor(x,240 - th*(up+1));
  if (fixCursor)
    CursorClear();
  tft.print(txt);
  if (tw < eraseTo)
    tft.fillRect(x+tw,240 - th*(up+1),eraseTo - tw, th,ILI9341_BLACK);
  eraseTo = tw;
  if (fixCursor)
    CursorRestore();
}


//=================================================================================================
void AudioPatcherDisplay::GetCursorSaveParams(const int16_t x, const int16_t y, 
                                              int16_t& cxr, int16_t& cyr, int16_t& cw, int16_t& ch)
{
  cxr = x - CURSOR_SIZE/2; cyr = y - CURSOR_SIZE/2;
  cw = ch = CURSOR_SIZE;

  if (cxr < 0)
  {
    cw += cxr;
    cxr = 0;
  }
  if (cyr < 0)
  {
    ch += cyr;
    cyr = 0;
  }
}


void AudioPatcherDisplay::GetCursor(int16_t& x, int16_t& y)
{
  x = cursor_x;
  y = cursor_y;
}


void AudioPatcherDisplay::CursorTo(int16_t x, int16_t y)
{
  int16_t cxr,cyr,cw,ch;

  if (nullptr != cbuf)
  {    
    GetCursorSaveParams(cursor_x,cursor_y,cxr,cyr,cw,ch);
    tft.writeRect(cxr,cyr,cw,ch,cbuf); // restore contents at old position
  }

  cursor_x = x;
  cursor_y = y;
  GetCursorSaveParams(cursor_x,cursor_y,cxr,cyr,cw,ch);
  tft.readRect(cxr,cyr,cw,ch,behindCursor); // stash content at new position
  cbuf = behindCursor;

  tft.drawRect(x-CURSOR_SIZE/2,y-1,CURSOR_SIZE,2,ILI9341_WHITE);
  tft.drawRect(x-1,y-CURSOR_SIZE/2,2,CURSOR_SIZE,ILI9341_WHITE);
}


void AudioPatcherDisplay::CursorClear(void)
{
  int16_t cxr,cyr,cw,ch;

  if (nullptr != cbuf)
  {    
    GetCursorSaveParams(cursor_x,cursor_y,cxr,cyr,cw,ch);
    tft.writeRect(cxr,cyr,cw,ch,cbuf); // restore contents at old position
    cbuf = nullptr; // invalidate
  }  
}


void AudioPatcherDisplay::CursorRestore(void)
{
  CursorTo(cursor_x,cursor_y);  
}


//=================================================================================================
void AudioPatcherDisplay::DrawPatchcord(AudioObjInstance& src, int8_t sp, AudioObjInstance& dst, int8_t dp, uint16_t colour)
{
  if (sp < src.objP->outputs && dp < dst.objP->inputs) // port numbers are valid
  {
    int16_t sx,sy,ss,dx,dy;

    // find positions of input and output connector blobs
    // these are at the top left
    getOutputPositions(*src.objP,src.x,src.y,&sx,&sy,&ss);
    sy += sp*ss;
    getInputPositions(*dst.objP,dst.x,dst.y,&dx,&dy,&ss);
    dy += dp*ss;

    // patchcords start outside the box
    sx += osize.cw + 1;
    dx -= 1;

    sy += osize.ch / 2;
    dy += osize.ch / 2;

    tft.drawLine(sx,sy,dx,dy,colour);
  }
}
