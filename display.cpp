#include "display.h"

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
AudioPatcherDisplay& display = AudioPatcherDisplay::getInstance();
uint16_t behindCursor[100];

static XPT2046_Touchscreen ts(TCH_CS);
AudioPatcherTouch touch{ts, 320, 240};

using namespace AudioPatcherBitDefs;

void AudioPatcherDisplay::Init(void)
{
  pinMode(TCH_CS,OUTPUT);
  digitalWriteFast(TCH_CS,HIGH);
  touch.begin();
  
  tft.begin();
  tft.setRotation(TFT_ROTATION);
  tft.setScroll(0);
  Clear();
  screenBuffer = (uint16_t*) extmem_malloc(tft.width() * tft.height() * sizeof *screenBuffer);
  screenReadOK();
}

bool AudioPatcherDisplay::screenReadOK(void)
{
  bool result = false;
  uint16_t testValues[] = {0x1234, 0x0811};

  if (nullptr != screenBuffer)
  {
    tft.drawPixel(0,0,testValues[0]);
    tft.drawPixel(0,1,testValues[1]);
    screenBuffer[0] = 0xDEAD;
    screenBuffer[1] = 0xBEEF;
    SaveArea(0,0,1,2);
    if (testValues[0] != screenBuffer[0] || testValues[1] != screenBuffer[1])
      Serial.printf("Screen read-back gave: 0x%04X, 0x%04X\n",
                     screenBuffer[0],
                     screenBuffer[1]);
    else 
    {
      Serial.println("Screen read-back OK");
      result = true;
    }
  }
  else
      Serial.println("No screen save buffer");

  return result;
}

void AudioPatcherDisplay::Clear(void)
{
  tft.fillScreen(ILI9341_BLACK);
}


bool AudioPatcherDisplay::SaveAreaToBuffer(int16_t x, int16_t y, int16_t w, int16_t h,  uint16_t* buf)
{
  bool result = false;
  
  if (nullptr != buf)
  {
    tft.readRect(x,y,w,h,buf);
    result = true;
  }

  return result;
}


void AudioPatcherDisplay::SaveArea(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (SaveAreaToBuffer(x,y,w,h,screenBuffer))
    savedArea = {x,y,w,h};
}

void AudioPatcherDisplay::GetArea(int16_t& x, int16_t& y, int16_t& w, int16_t& h)
{
  x = savedArea.x;
  y = savedArea.y;
  w = savedArea.w;
  h = savedArea.h;
}

void AudioPatcherDisplay::RestoreArea(void)
{
  if (nullptr != screenBuffer)
  {
    tft.writeRect(savedArea.x,savedArea.y,savedArea.w,savedArea.h,screenBuffer);
  }
}


void AudioPatcherDisplay::InitArea(int16_t x, int16_t y, int16_t w, int16_t h, bool withHeader /* = true */)
{
  tft.fillRoundRect(x,y,w,h,6,EDIT_BKGND); 
  if (withHeader)
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
}

void AudioPatcherDisplay::ShowVoiceFlag(bool flag)
{
  int16_t colour = flag?0xA514:ILI9341_BLACK;
  const int16_t cr = 4;
  
  tft.fillCircle(savedArea.x + savedArea.w - 5 - cr,savedArea.y + 5 + cr, cr, colour); // assume we're operating in the area we saved
}

void AudioPatcherDisplay::ShowLabel(const ParamEntry& p, ParamValue& v, int16_t n, int16_t xoff, int16_t yoff)
{
  if (nullptr != p.label)
  {
    int xxoff = xoff + p.xoff;
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_LIGHTGREY,EDIT_BKGND);
    tft.setCursor(savedArea.x + xxoff,savedArea.y + n*16 + yoff); // assume we're operating in the area we saved
    tft.print(p.label);
    tft.print(":");
    
    tft.getCursor(&v.labelEndX,&v.labelEndY);
  }
}

void AudioPatcherDisplay::FillRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t colour) 
{ 
  tft.fillRect(x,y,w,h,colour); 
}


void AudioPatcherDisplay::ShowValue(const ParamEntry& p, ParamValue& v, int16_t n)
{
  if (nullptr != p.label)
  {
    tft.setCursor(v.labelEndX,v.labelEndY);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_LIGHTGREY,EDIT_BKGND);
    switch (p.ValType)
    {
      default: tft.print("???"); break;
      case 'i': tft.print(v.value.i); break;
      case 'n':
      case 'f': tft.printf("%.2f",v.value.f); break;
      case 'c': tft.print(p.choices[v.value.i].text); break;
      case 'l': tft.printf("%.2f",pow(2.0f,v.value.f)); break;
      case 'r': tft.printf("%.2f",p.min.f / v.value.i); break;
    }
  
    int16_t x,y;
    tft.getCursor(&x,&y);
    if (v.valueEndX > x)
    {
      tft.fillRect(x,v.labelEndY,v.valueEndX - x, 16, EDIT_BKGND);
    }
    v.valueEndX = x;  
  }
}

void AudioPatcherDisplay::Splash(void)
{
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.print("Here we ");
  delay(100);
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
  delay(400);
}

struct AudioObjectColours_s {uint16_t body,border,text; }
  AudioObjectColours[] = {{ILI9341_BLACK, ILI9341_WHITE, ILI9341_YELLOW}, // none
                          {CONNECTION_COLOUR, PATCHCORD_COLOUR, ILI9341_WHITE}, // connection
                          {ILI9341_PURPLE, 0xD01C, ILI9341_WHITE},
                          {ILI9341_NAVY, 0x03FF, ILI9341_WHITE},
                          {0x0280, ILI9341_GREEN, ILI9341_WHITE},
                          {0x3800, ILI9341_RED, ILI9341_WHITE},
                          {0x04200, ILI9341_YELLOW, ILI9341_WHITE}, // synth
                          {ILI9341_BLACK, ILI9341_DARKGREY, ILI9341_WHITE}, // \  ---------------
                          {ILI9341_BLACK, ILI9341_DARKGREY, ILI9341_WHITE}, //  } I/O and control
                          {ILI9341_BLACK, ILI9341_DARKGREY, ILI9341_WHITE}, // /  ---------------
                         };
struct AudioObjectSizes_t {
  const int ow,oh,cc,cw,ch,ff;                          
} osize = {.ow = 46,.oh = 46, .cc = 4, .cw = 3, .ch = 3, .ff = 6};


//=================================================================================================
bool AudioPatcherDisplay::objIsOnScreen(int16_t x, int16_t y, int16_t w, int16_t h)
{
  return
      x < tft.width() // check object is visible
   && 0 < x + w
   && y < tft.height()
   && 0 < y + h;  
}


//=================================================================================================
AudioPatcherDisplay::side AudioPatcherDisplay::PointIsInObj(AudioObjInstance& aoi, int16_t x, int16_t y)
{
  side result = side::out;
  // transform screen point to canvas co-ordinates:
  x += canvas_x;
  y += canvas_y;

  if (aoi.x <= x && aoi.x+osize.ow >= x
   && aoi.y <= y && aoi.y+osize.oh >= y)
  {
    if (x - aoi.x < osize.ow/2)
      result = side::left;
    else
      result = side::right;
  }

  return result;
}


int16_t AudioPatcherDisplay::PointDistanceToPatchcord(PatchcordInstance_t& cord, int16_t x, int16_t y)
{
  
  int16_t sx, sy, dx, dy, xy;
  float a,b,c,ab2, result;
  bool offline = false;
  
  getPatchcordEnds(*cord.src, cord.src_port, *cord.dst, cord.dst_port, true,  sx, sy, dx, dy);

  a = (float)(sy-dy)/(float)(sx-dx);
  b = -1.0f;
  c = sy - a*sx;
  ab2 = a*a+b*b;
  
  //Serial.printf("Point: %d,%d; cord: %d,%d -> %d,%d; %.3fx + %.3fy + %.3f = 0; ", x,y, sx,sy,dx,dy, a,b,c);

  result = fabs((a*x + b*y + c) / sqrt(ab2));

  // see if we're past ends of cord
  xy = (b*(b*x-a*y)-a*c)/ab2; // closest X on line
  if (sx < dx && (xy < sx || xy > dx)) offline = true; 
  if (sx > dx && (xy > sx || xy < dx)) offline = true; 
  
  xy = (a*(-b*x+a*y)-b*c)/ab2; // closest Y
  if (sy < dy && (xy < sy || xy > dy)) offline = true; 
  if (sy > dy && (xy > sy || xy < dy)) offline = true; 

  if (offline)
    result = -result;
  
  return (int16_t) result;
}

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


bool AudioPatcherDisplay::getPatchcordEnds(AudioObjInstance& src, int8_t sp, AudioObjInstance& dst, int8_t dp, 
                                           bool convertToScreen,
                                           int16_t& sx, int16_t& sy, int16_t& dx, int16_t& dy)
{
  bool result = false;
  
  if (sp < src.objP->outputs && dp < dst.objP->inputs) // port numbers are valid
  {
    int16_t ss;

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

    if (convertToScreen)
    {
      sx -= canvas_x; sy -= canvas_y;
      dx -= canvas_x; dy -= canvas_y;    
    }
    result = true;
  }

  return result;
}


//=================================================================================================
bool AudioPatcherDisplay::DrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y, bool greyed)
{ 
  bool result = false;
  x -= canvas_x; y -= canvas_y;

  if (objIsOnScreen(x,y))
  {
    uint16_t bc = greyed?ILI9341_DARKGREY:AudioObjectColours[o.category].body;
    uint16_t ec = greyed?ILI9341_LIGHTGREY:AudioObjectColours[o.category].border;
  
    tft.fillRoundRect(x,y,osize.ow,osize.oh,osize.cc,bc);
    tft.drawRoundRect(x,y,osize.ow,osize.oh,osize.cc,ec);
    
    int16_t lw,lh;
    tft.setFontAdafruit();
    tft.setTextSize(1);
    tft.setTextWrap(false);
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

    result = true;
  }
  return result;
}

void AudioPatcherDisplay::DrawPerVoice(AudioObjInstance& aoi, bool greyed)
{ 
    uint16_t ec = greyed?ILI9341_DARKGREY:AudioObjectColours[aoi.objP->category].body; // assume not per-voice
    int16_t x = aoi.x - canvas_x + OBJECT_SIZE - 11, y = aoi.y - canvas_y + 6;
    
    if (aoi.perVoice)
      ec = greyed?ILI9341_LIGHTGREY:AudioObjectColours[aoi.objP->category].border;
    
    if (objIsOnScreen(x,y))
      tft.fillCircle(x,y, 3, ec);
}

void AudioPatcherDisplay::DrawAudioObject(AudioObjInstance& aoi, bool greyed)
{ 
  greyed = greyed || aoi.drawInGrey; // no ||= operator in C :(

  if (DrawAudioObject(*aoi.objP,aoi.x,aoi.y,greyed))
    DrawPerVoice(aoi,greyed); 
}


void AudioPatcherDisplay::EraseAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y)
{
  x -= canvas_x; y -= canvas_y;
  if (objIsOnScreen(x,y))
    tft.fillRoundRect(x-1,y-1,osize.ow+2,osize.oh+2,osize.cc+1,ILI9341_BLACK);   
}


//=================================================================================================
void AudioPatcherDisplay::HighlightAudioObject(int16_t x, int16_t y, uint16_t colour)
{
  x -= canvas_x; y -= canvas_y;
  
  if (objIsOnScreen(x,y))
    tft.drawRoundRect(x-1,y-1,osize.ow+2,osize.oh+2,osize.cc+1,colour);  
}


void AudioPatcherDisplay::HighlightAudioObject(int16_t x, int16_t y, bool on)
{
  HighlightAudioObject(x,y,(uint16_t)(on?ILI9341_WHITE:ILI9341_BLACK));  
}


//=================================================================================================
void AudioPatcherDisplay::DrawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n , bool op, uint16_t colour)
{
  x -= canvas_x; y -= canvas_y;
  
#define BAD -999  
  if (objIsOnScreen(x,y))
  {
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
  int16_t ty = tft.height() - th;
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(1,ty);

  tft.fillRect(0,ty-1,tft.measureTextWidth(txt,1)+1,th+1,colour);
  tft.print(*txt);
}


//=================================================================================================
void AudioPatcherDisplay::ShowBottomText(const char* txt, int colour /* = modeColour */)
{
  static int16_t eraseTo = -1;
  bool fixCursor;

  if (NOT_A_COLOUR == colour)
    colour = modeColour;
  
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

void AudioPatcherDisplay::ShowAreaText(const char* txt, int xoff, int yoff, int row, int fg, int bg)
{
  int eraseTo = savedArea.w - xoff - 5;
  xoff += savedArea.x;
  
  tft.setFontAdafruit();
  tft.setTextSize(2);
  int16_t tw = tft.measureTextWidth(txt),
          th = tft.fontLineSpace()+AREA_EXTRA; // assume it's going to fit on one line!
  yoff += savedArea.y + row*th;
  tft.setTextColor(fg,bg);
  tft.setCursor(xoff,yoff);
  tft.print(txt);
  tft.fillRect(xoff,yoff+th-AREA_EXTRA,tw,AREA_EXTRA, bg);
  tft.fillRect(xoff+tw,yoff,eraseTo - tw,th, bg);
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

void AudioPatcherDisplay::SaveStatus(void)
{
  SaveArea(0,tft.height() - 17,tft.width(),17);  
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
    int16_t sx,sy,dx,dy;

    // find positions of input and output connector blobs
    if (getPatchcordEnds(src, sp, dst, dp, true, sx, sy, dx, dy))
      tft.drawLine(sx,sy,dx,dy,colour);
  }
}

//=================================================================================================
void AudioPatcherDisplay::canvasMoveBy(int16_t x, int16_t y)
{
  canvas_x += x;
  canvas_y += y;
  CursorClear();
  tft.fillRect(0,0,320,223,ILI9341_BLACK);
}


void AudioPatcherDisplay::canvasMoveTo(int16_t x, int16_t y)
{
  canvas_x = x;
  canvas_y = y;
  canvasMoveBy(0,0);
}

void AudioPatcherDisplay::canvasGetLimits(int16_t& xmax, int16_t& ymax) 
{ 
  xmax = tft.width(); 
  ymax = tft.height(); 
}


bool AudioPatcherDisplay::canvasMakeVisible(int16_t x, int16_t y, int16_t w, int16_t h, int16_t xstep, int16_t ystep)
{
  bool result = false; // assume we're not needing to move
  int16_t xmax,ymax,xmove=0,ymove=0;

  canvasGetLimits(xmax, ymax);
  ymax -= 20; // allow for status zone (magic number)
  
  while (x - canvas_x + w - xmove > xmax) // moved object RHS is off screen
    xmove += xstep;
  while (x - canvas_x - xmove < 0) // moved object LHS is off screen
    xmove -= xstep;

  while (y - canvas_y + h - ymove > ymax) // moved object bottom is off screen
    ymove += ystep;
  while (y - canvas_y - ymove < 0) // moved object top is off screen
    ymove -= xstep;

  // see if move is needed...
  result = 0 != xmove || 0 != ymove;
  if (result)
    canvasMoveBy(xmove,ymove); // ...and do it
  
  return result; // true if objects need re-drawing
}


bool AudioPatcherDisplay::canvasMakeVisible(AudioObjInstance& obj, int16_t xstep, int16_t ystep)
{
  return canvasMakeVisible(obj.x, obj.y, OBJECT_SIZE, OBJECT_SIZE, xstep, ystep); 
}


bool AudioPatcherDisplay::canvasMakeVisible(PatchcordInstance_t& cord, int16_t xstep, int16_t ystep)
{
    int16_t sx,sy,ss,dx,dy;

    // find positions of input and output connector blobs
    // these are at the top left
    getOutputPositions(*cord.src->objP,cord.src->x,cord.src->y,&sx,&sy,&ss);
    sy += cord.src_port * ss;
    getInputPositions(*cord.dst->objP,cord.dst->x,cord.dst->y,&dx,&dy,&ss);
    dy += cord.dst_port * ss;

    if (sx > dx)
      std::swap(sx,dx);
      
    if (sy > dy)
      std::swap(sy,dy);
      
    return canvasMakeVisible(sx, sy, dx - sx, dy - sy, xstep, ystep); 
}

//=================================================================================================
static const char* key_rows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "_zxcvbnm-"};
using namespace AudioPatcherBitDefs;
static uint8_t 
delKey[] =
{// W   H
   21, 19,
  _______XX,_XXXXXXXX,_XXXXX___,
  ______XXX,_XXXXXXXX,_XXXXX___,
  _____XX__,_________,____XX___,
  ____XX___,_________,____XX___,
  ____XX___,_________,____XX___,
  ___XX___X,_X______X,_X__XX___,
  ___XX____,_XX____XX,____XX___,
  __XX_____,__XX__XX_,____XX___,
  __XX_____,___XXXX__,____XX___,
  _XX______,____XX___,____XX___,
  __XX_____,___XXXX__,____XX___,
  __XX_____,__XX__XX_,____XX___,
  ___XX____,_XX____XX,____XX___,
  ___XX___X,_X______X,_X__XX___,
  ____XX___,_________,____XX___,
  ____XX___,_________,____XX___,
  _____XX__,_________,____XX___,
  ______XXX,_XXXXXXXX,_XXXXX___,
  _______XX,_XXXXXXXX,_XXXXX___,
},
entKey[] =
{// W   H
   21, 19,
  _________,_________,__XXXX___,
  _________,_________,__XXXX___,
  _________,_________,__XXXX___,
  _________,_________,__XXXX___,
  _________,_________,__XXXX___,
  _________,_________,__XXXX___,
  _________,_________,__XXXX___,
  _______XX,_________,__XXXX___,
  ______XX_,_________,__XXXX___,
  _____XX__,_________,__XXXX___,
  ____XX___,_________,__XXXX___,
  ___XXXXXX,_XXXXXXXX,_XXXXX___,
  __XXXXXXX,_XXXXXXXX,_XXXXX___,
  ___XXXXXX,_XXXXXXXX,_XXXX____,
  ____XX___,_________,_________,
  _____XX__,_________,_________,
  ______XX_,_________,_________,
  _______XX,_________,_________,
  _________,_________,_________,
},
shfKey[] =
{// W   H
   24, 19,
  _________,_________,_________,
  _________,_________,_________,
  _____XXXX,_XX______,___XX____,
  ____XXXXX,_XXX_____,___XX____,
  _________,___XX____,___XX____,
  _________,___XX____,__XXXX___,
  ____XXXXX,_XXXX____,__XXXX___,
  ___XXXXXX,_XXXX____,__XXXX___,
  ___XX____,___XX____,_XX__XX__,
  ___XX____,__XXX____,_XX__XX__,
  ___XXXXXX,_XX_XX___,_XX__XX__,
  ____XXXXX,_X___XX_X,_X____XX_,
  _________,________X,_XXXXXXX_,
  _________,________X,_XXXXXXX_,
  _________,_______XX,_______XX,
  _________,_______XX,_______XX,
  _________,_______XX,_______XX,
  _________,_________,_________,
  _________,_________,_________,
}
;

#define KEY_SZ 25 
static std::vector<BitmapKey> keysVec = {
  {5*KEY_SZ, 4*KEY_SZ+6, -10, shfKey},
  {7*KEY_SZ, 4*KEY_SZ+6, -11, delKey},
  {8*KEY_SZ, 4*KEY_SZ+6, -12, entKey},
};

void AudioPatcherDisplay::keypos(int16_t x, int16_t y, size_t r, size_t c,
                                 int16_t& xoff, int16_t& yoff)
{
  xoff = x + c*KEY_SIZE + r*KEY_STAGGER + 6;
  yoff = y+r*KEY_SIZE+2;  
}

char AudioPatcherDisplay::ShowKey(int16_t x, int16_t y, size_t r, size_t c, int16_t fg, int16_t bg, bool upr /* = false */)
{
  char result = key_rows[r][c];
  int16_t xoff, yoff;

  keypos(x,y,r,c,xoff,yoff);
  
  if (r > 0 && upr)
    result = result & ~0x20;

  tft.fillRoundRect(xoff,yoff,KEY_SIZE-2,KEY_SIZE-2,4,bg);
  tft.drawRoundRect(xoff,yoff,KEY_SIZE-2,KEY_SIZE-2,4,fg);
  tft.drawChar(xoff+6,y+r*KEY_SIZE+6,result,fg,bg,2);

  // store keyboard position ready for readback
  if (0 == r && 0 == c)
  {
    int16_t xoff2, yoff2;
  
    keypos(0,0,0,0,xoff2,yoff2);
    keyboard_x = xoff - xoff2;
    keyboard_y = yoff- yoff2;
  }

  return result;
}


char AudioPatcherDisplay::ShowKey(size_t r, size_t c, int16_t fg, int16_t bg, bool upr /* = false */)
{
  return ShowKey(keyboard_x,keyboard_y,r,c,fg,bg,upr);  
}


char AudioPatcherDisplay::ShowKey(keyInfo key, int16_t fg, int16_t bg, bool upr /* = false */)
{
  char result = 0;

  if (key.ch > 0)
    result = ShowKey(keyboard_x,keyboard_y,key.row,key.col,fg,bg,upr);
  else
  {
    for (auto bk : keysVec)
      if (bk.hitVal == key.ch)
      {
        result = ShowBitmapKey(bk.xpos, bk.ypos, fg, bg, bk.bitmap);      
        break;
      }
  }

  return result;
}


char AudioPatcherDisplay::ShowBitmapKey(int16_t x, int16_t y, int16_t fg, int16_t bg, const uint8_t* bitmap)
{
  int16_t xoff = x + keyboard_x;
  int16_t yoff = y + keyboard_y;

  tft.fillRoundRect(xoff-1,yoff-1,bitmap[0]+2,bitmap[1]+2,4,bg);
  tft.drawBitmap(xoff,yoff,
                bitmap+2,bitmap[0],bitmap[1],
                fg);
  return 0;                
}


void AudioPatcherDisplay::RedrawKeyboard(int16_t x, int16_t y, bool upperCase)
{
  keyboard_uppercase = upperCase;
  
  for (size_t r = 0; r < COUNT_OF(key_rows); r++)
  {
    size_t cols = strlen(key_rows[r]);
    for (size_t c = 0; c < cols;c++)
      ShowKey(x,y,r,c,KEY_CAP_COLOUR,EDIT_BKGND,upperCase);
  }

  for (auto key : keysVec)
    ShowBitmapKey(key.xpos,key.ypos,KEY_CAP_COLOUR,EDIT_BKGND,key.bitmap);  
}


void AudioPatcherDisplay::ShowKeyboard(int16_t x, int16_t y, 
                                       const char* title, /* = nullptr */
                                       bool saveArea /* = true */)
{
  bool hasTitle = nullptr != title;

  if (saveArea)
    SaveArea(x,y,KEY_SIZE*11 + 4,KEY_SIZE*4 + 4 + 30 + (hasTitle?25:0));
  InitArea(savedArea.x,savedArea.y,savedArea.w,savedArea.h,hasTitle);

  if (hasTitle)
  {
    ShowTitle(title,5,5);
    y += 27;  
  }

  RedrawKeyboard(x,y,false);
}


AudioPatcherDisplay::keyInfo AudioPatcherDisplay::KeyAt(int16_t x, int16_t y)
{
  keyInfo result = {0}; // assume a miss

  x -= keyboard_x; // make y keyboard-relative
  y -= keyboard_y; 

  for (auto key : keysVec)
  {
    if (x >= key.xpos 
     && x <= key.xpos + key.bitmap[0]
     && y >= key.ypos 
     && y <= key.ypos + key.bitmap[1]
     )
    {
      result = {key.hitVal,y,x};
      break;
    }
  }

  if (0 == result.ch)
  {
    y /= KEY_SIZE;   // and into row number
    if (y >= 0 && y < (int16_t) COUNT_OF(key_rows)) // in a row, at least!
    {
      x -= y*KEY_STAGGER; // x becomes row-relative
      x /= KEY_SIZE;      // ...and column number
      if (x >=0 && x < (int16_t) strlen(key_rows[y]))
        result = {key_rows[y][x], y, x};
    }
  }

  if (keyboard_uppercase 
   && result.ch >= 'a' && result.ch <= 'z') // don't uppercase numbers!
    result.ch &= ~0x20;
    
  return result;
}

AudioPatcherDisplay::keyInfo AudioPatcherDisplay::LineAt(int16_t x, int16_t y, int16_t xoff, int16_t yoff)
{
  keyInfo result = {-99}; // assume a miss

  int16_t xr = x - savedArea.x - xoff;
  
  if (xr >= 0 && xr <= savedArea.w)
  {
    // need to set font to get correct sizes
    tft.setFontAdafruit();
    tft.setTextSize(2);
    int16_t th = tft.fontLineSpace()+AREA_EXTRA; // assume it's going to fit on one line!  
    int16_t yr = y - savedArea.y - yoff;
    if (yr < 0)
      yr -= th;
    yr /= th;

    result = {yr,x,y};
  }

  return result;
}
//=================================================================================================
bool AudioPatcherTouch::isTouched(void)
{ 
  bool result;
  
  touch_shift = (touch_shift<<1) | ts.touched();
  result = (touch_shift & 7) == 7;
  if (result)
  {
    lastPoint = getPoint();
    penState = down;
  }
  else
  {
    if (down == penState)
      penState = lifted;
  }
   
  return result;
}
