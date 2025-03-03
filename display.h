#if !defined(_DISPLAY_H_)
#define _DISPLAY_H_

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"
#include "objects.h"
#include "paramEntry.h"


#define TFT_DC  9
#define TFT_CS 22
#define TCH_CS  5
#define TFT_ROTATION 1
#define TCH_ROTATION 3

#define NOT_A_COLOUR 0x1DEAD
/* // Values for 2.4" display
#define CONNECTION_COLOUR    0x9492 // ILI9341_DARKGREY
#define PATCHCORD_COLOUR     0xFC02 // orange
#define PATCHCORD_HIGHLIGHT  0xFE04 // orange
#define EDIT_BKGND ILI9341_DARKGREY
*/

// Values for 3.2" display
#define CONNECTION_COLOUR    0x9492 // ILI9341_DARKGREY
#define PATCHCORD_COLOUR     0xFA02 // orange
#define PATCHCORD_HIGHLIGHT  0xFC04 // orange
#define EDIT_BKGND           0x528A // darker grey
#define KEY_CAP_COLOUR       ILI9341_LIGHTGREY
#define DIR_NAME_COLOUR      0x27E4 // pale green
#define KEY_ACTIVE_BKGND     0x1042 // very dark grey

//A 320x240x16-bit display takes 153,600 bytes to store
class AudioPatcherDisplay
{
    // make this a singleton: private / inaccessible constructor / destructors...
    AudioPatcherDisplay()  = default;
    ~AudioPatcherDisplay() = default;
  public:
    // ...delete the copy semantics...
    AudioPatcherDisplay(const AudioPatcherDisplay&) = delete;
    AudioPatcherDisplay& operator= (const AudioPatcherDisplay&) = delete;
    // ...and provide an instance generator
    static AudioPatcherDisplay& getInstance()
    {
      static AudioPatcherDisplay theOnlyOneEver;
      return theOnlyOneEver;
    }
    
  private:    
    uint16_t* cbuf = nullptr; 
    int16_t cursor_x; 
    int16_t cursor_y;
    int16_t canvas_x; 
    int16_t canvas_y;
    int16_t keyboard_x; 
    int16_t keyboard_y;
    bool keyboard_uppercase;
    uint16_t modeColour;
    uint16_t* screenBuffer = nullptr;
    struct {int16_t x,y,w,h;} savedArea;
    static const int16_t CURSOR_SIZE = 10;
    static const int16_t OBJECT_SIZE = 48;
    static const int16_t KEY_SIZE = 25;
    static const int16_t KEY_STAGGER = KEY_SIZE / 3;
    
    void GetCursorSaveParams(const int16_t x, const int16_t y, int16_t& cxr, int16_t& cyr, int16_t& cw, int16_t& ch);
    bool objIsOnScreen(int16_t x, int16_t y, int16_t w = OBJECT_SIZE, int16_t h = OBJECT_SIZE); // call after canvas co-ordinate transformation
    bool screenReadOK(void);
    void keypos(int16_t x, int16_t y, size_t r, size_t c, int16_t& xoff, int16_t& yoff);
    
  public:  
    void Init(void);
    void Clear(void);
    void Splash(void);

    // canvas operations
    void canvasMoveBy(int16_t x, int16_t y);
    void canvasMoveTo(int16_t x, int16_t y);
    void canvasGetCurrent(int16_t& x, int16_t& y) { x = canvas_x; y = canvas_y; }
    bool canvasMakeVisible(int16_t x, int16_t y, int16_t w, int16_t h, int16_t xstep, int16_t ystep);
    bool canvasMakeVisible(AudioObjInstance& obj, int16_t xstep, int16_t ystep);
    bool canvasMakeVisible(PatchcordInstance_t& cord, int16_t xstep, int16_t ystep);
    void canvasGetLimits(int16_t& xmax, int16_t& ymax);
    
    // dependent on display's window on patcher canvas:
    bool DrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y, bool greyed = false); // return true if we drew it
    void DrawAudioObject(AudioObjInstance& aoi, bool greyed = false);
    void DrawPerVoice(AudioObjInstance& aoi, bool greyed = false);
    void EraseAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y);
    void HighlightAudioObject(int16_t x, int16_t y, bool on = true);
    void HighlightAudioObject(int16_t x, int16_t y, uint16_t colour);
    void DrawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n = 0, bool op = false, uint16_t colour = CONNECTION_COLOUR);
    void DrawPatchcord(AudioObjInstance& src, int8_t sp, AudioObjInstance& dst, int8_t dp, uint16_t colour = PATCHCORD_COLOUR);
    void DrawPatchcord(PatchcordInstance_t* cord, uint16_t colour = PATCHCORD_COLOUR) { DrawPatchcord(*cord->src,cord->src_port,*cord->dst,cord->dst_port, colour); }
    bool PointIsInObj(AudioObjInstance& aoi, int16_t x, int16_t y);
    int16_t PointDistanceToPatchcord(PatchcordInstance_t& cord, int16_t x, int16_t y);
    
    // bottom line doesn't move with canvas
    void ShowMode(const char* txt);
    void ShowSelection(const char* txt, AudioCategory_e cat);
    void ShowBottomText(const char* txt, int colour = NOT_A_COLOUR);
    void ShowStatus(const char* txt,int16_t x,int16_t y, uint16_t colour);
    void ShowAreaText(const char* txt, int xoff, int yoff, int row, int fg, int bg);
    void SaveStatus(void);
    void RestoreStatus(void) { RestoreArea(); }

    // cursor is always relative to screen
    void CursorSave(void);
    void CursorClear(void);
    void CursorRestore(void);
    void CursorTo(int16_t x, int16_t y);
    void GetCursor(int16_t& x, int16_t& y);
    
    uint16_t getModeColour(void) { return modeColour; }
    bool getPatchcordEnds(AudioObjInstance& src, int8_t sp, AudioObjInstance& dst, int8_t dp, 
                          bool convertToScreen,
                          int16_t& sx, int16_t& sy, int16_t& dx, int16_t& dy);
    
    // parameter settings box is screen-relative
    void SaveArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void InitArea(int16_t x, int16_t y, int16_t w, int16_t h, bool withHeader = true);
    void GetArea(int16_t& x, int16_t& y, int16_t& w, int16_t& h);
    void RestoreArea(void);
    void ShowTitle(const char* t, int16_t xoff, int16_t yoff);
    void ShowVoiceFlag(bool flag);
    void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t colour);
    void ShowLabel(const ParamEntry& p, ParamValue& v, int16_t n, int16_t xoff, int16_t yoff);
    void ShowValue(const ParamEntry& p, ParamValue& v, int16_t n);

    // keyboard
    struct keyInfo {int ch, row, col;};
    char ShowKey(int16_t x, int16_t y, size_t r, size_t c, int16_t fg, int16_t bg, bool upr = false);
    char ShowKey(size_t r, size_t c, int16_t fg, int16_t bg, bool upr = false);
    char ShowKey(keyInfo key, int16_t fg, int16_t bg, bool upr = false);
    char ShowBitmapKey(int16_t x, int16_t y, int16_t fg, int16_t bg, const uint8_t* bitmap);
    void ShowKeyboard(int16_t x, int16_t y, const char* title = nullptr,bool saveArea = true);
    void RedrawKeyboard(int16_t x, int16_t y, bool upperCase);
    void RedrawKeyboard(bool upperCase) { RedrawKeyboard(keyboard_x, keyboard_y, upperCase); }
    keyInfo KeyAt(int16_t x, int16_t y);
};


extern AudioPatcherDisplay& display;

#include "XPT2046_Touchscreen.h"

class AudioPatcherTouch
{
    XPT2046_Touchscreen& ts;
    int screen_width, screen_height;
    int xl, yl, xh, yh;
    uint32_t touch_shift;
    TS_Point lastPoint;
    enum {up, down, lifted} penState;
  public:
    AudioPatcherTouch(XPT2046_Touchscreen& _ts, int w, int h)
    : ts(_ts), screen_width(w), screen_height(h),
    xl(200), yl(200), xh(3800), yh(3800), // should be calibrated at some point
    touch_shift(0)
    {}
    bool begin(void) {bool result = ts.begin(); ts.setRotation(TCH_ROTATION); return result; }
    bool isTouched(void);
    bool isLifted(void) 
    { 
      bool result = false; 
      if (lifted == penState) 
      {
        penState = up; 
        result = true; 
      } 
      return result;
    }
    
    TS_Point getLastPoint(void) { return lastPoint; }
    TS_Point getPoint(void)
    {
      TS_Point p = ts.getPoint(); // int16_t values

      //Serial.printf("%d,%d -> ",p.x,p.y);
      p.x = map(p.x, xl, xh, 0, screen_width);
      p.y = map(p.y, yl, yh, 0, screen_height);
      p.x = constrain(p.x, 0, screen_width);
      p.y = constrain(p.y, 0, screen_height); 
      //Serial.printf("%d,%d\n",p.x,p.y);

      return p;
    }
};

struct BitmapKey
{
  int xpos,ypos,hitVal;
  uint8_t* bitmap;
};

extern AudioPatcherTouch touch;

namespace AudioPatcherBitDefs {
const uint8_t _________ = 0;
const uint8_t ________X = 1;
const uint8_t _______X_ = 2;
const uint8_t _______XX = 3;
const uint8_t ______X__ = 4;
const uint8_t ______X_X = 5;
const uint8_t ______XX_ = 6;
const uint8_t ______XXX = 7;
const uint8_t _____X___ = 8;
const uint8_t _____X__X = 9;
const uint8_t _____X_X_ = 10;
const uint8_t _____X_XX = 11;
const uint8_t _____XX__ = 12;
const uint8_t _____XX_X = 13;
const uint8_t _____XXX_ = 14;
const uint8_t _____XXXX = 15;
const uint8_t ____X____ = 16;
const uint8_t ____X___X = 17;
const uint8_t ____X__X_ = 18;
const uint8_t ____X__XX = 19;
const uint8_t ____X_X__ = 20;
const uint8_t ____X_X_X = 21;
const uint8_t ____X_XX_ = 22;
const uint8_t ____X_XXX = 23;
const uint8_t ____XX___ = 24;
const uint8_t ____XX__X = 25;
const uint8_t ____XX_X_ = 26;
const uint8_t ____XX_XX = 27;
const uint8_t ____XXX__ = 28;
const uint8_t ____XXX_X = 29;
const uint8_t ____XXXX_ = 30;
const uint8_t ____XXXXX = 31;
const uint8_t ___X_____ = 32;
const uint8_t ___X____X = 33;
const uint8_t ___X___X_ = 34;
const uint8_t ___X___XX = 35;
const uint8_t ___X__X__ = 36;
const uint8_t ___X__X_X = 37;
const uint8_t ___X__XX_ = 38;
const uint8_t ___X__XXX = 39;
const uint8_t ___X_X___ = 40;
const uint8_t ___X_X__X = 41;
const uint8_t ___X_X_X_ = 42;
const uint8_t ___X_X_XX = 43;
const uint8_t ___X_XX__ = 44;
const uint8_t ___X_XX_X = 45;
const uint8_t ___X_XXX_ = 46;
const uint8_t ___X_XXXX = 47;
const uint8_t ___XX____ = 48;
const uint8_t ___XX___X = 49;
const uint8_t ___XX__X_ = 50;
const uint8_t ___XX__XX = 51;
const uint8_t ___XX_X__ = 52;
const uint8_t ___XX_X_X = 53;
const uint8_t ___XX_XX_ = 54;
const uint8_t ___XX_XXX = 55;
const uint8_t ___XXX___ = 56;
const uint8_t ___XXX__X = 57;
const uint8_t ___XXX_X_ = 58;
const uint8_t ___XXX_XX = 59;
const uint8_t ___XXXX__ = 60;
const uint8_t ___XXXX_X = 61;
const uint8_t ___XXXXX_ = 62;
const uint8_t ___XXXXXX = 63;
const uint8_t __X______ = 64;
const uint8_t __X_____X = 65;
const uint8_t __X____X_ = 66;
const uint8_t __X____XX = 67;
const uint8_t __X___X__ = 68;
const uint8_t __X___X_X = 69;
const uint8_t __X___XX_ = 70;
const uint8_t __X___XXX = 71;
const uint8_t __X__X___ = 72;
const uint8_t __X__X__X = 73;
const uint8_t __X__X_X_ = 74;
const uint8_t __X__X_XX = 75;
const uint8_t __X__XX__ = 76;
const uint8_t __X__XX_X = 77;
const uint8_t __X__XXX_ = 78;
const uint8_t __X__XXXX = 79;
const uint8_t __X_X____ = 80;
const uint8_t __X_X___X = 81;
const uint8_t __X_X__X_ = 82;
const uint8_t __X_X__XX = 83;
const uint8_t __X_X_X__ = 84;
const uint8_t __X_X_X_X = 85;
const uint8_t __X_X_XX_ = 86;
const uint8_t __X_X_XXX = 87;
const uint8_t __X_XX___ = 88;
const uint8_t __X_XX__X = 89;
const uint8_t __X_XX_X_ = 90;
const uint8_t __X_XX_XX = 91;
const uint8_t __X_XXX__ = 92;
const uint8_t __X_XXX_X = 93;
const uint8_t __X_XXXX_ = 94;
const uint8_t __X_XXXXX = 95;
const uint8_t __XX_____ = 96;
const uint8_t __XX____X = 97;
const uint8_t __XX___X_ = 98;
const uint8_t __XX___XX = 99;
const uint8_t __XX__X__ = 100;
const uint8_t __XX__X_X = 101;
const uint8_t __XX__XX_ = 102;
const uint8_t __XX__XXX = 103;
const uint8_t __XX_X___ = 104;
const uint8_t __XX_X__X = 105;
const uint8_t __XX_X_X_ = 106;
const uint8_t __XX_X_XX = 107;
const uint8_t __XX_XX__ = 108;
const uint8_t __XX_XX_X = 109;
const uint8_t __XX_XXX_ = 110;
const uint8_t __XX_XXXX = 111;
const uint8_t __XXX____ = 112;
const uint8_t __XXX___X = 113;
const uint8_t __XXX__X_ = 114;
const uint8_t __XXX__XX = 115;
const uint8_t __XXX_X__ = 116;
const uint8_t __XXX_X_X = 117;
const uint8_t __XXX_XX_ = 118;
const uint8_t __XXX_XXX = 119;
const uint8_t __XXXX___ = 120;
const uint8_t __XXXX__X = 121;
const uint8_t __XXXX_X_ = 122;
const uint8_t __XXXX_XX = 123;
const uint8_t __XXXXX__ = 124;
const uint8_t __XXXXX_X = 125;
const uint8_t __XXXXXX_ = 126;
const uint8_t __XXXXXXX = 127;
const uint8_t _X_______ = 128;
const uint8_t _X______X = 129;
const uint8_t _X_____X_ = 130;
const uint8_t _X_____XX = 131;
const uint8_t _X____X__ = 132;
const uint8_t _X____X_X = 133;
const uint8_t _X____XX_ = 134;
const uint8_t _X____XXX = 135;
const uint8_t _X___X___ = 136;
const uint8_t _X___X__X = 137;
const uint8_t _X___X_X_ = 138;
const uint8_t _X___X_XX = 139;
const uint8_t _X___XX__ = 140;
const uint8_t _X___XX_X = 141;
const uint8_t _X___XXX_ = 142;
const uint8_t _X___XXXX = 143;
const uint8_t _X__X____ = 144;
const uint8_t _X__X___X = 145;
const uint8_t _X__X__X_ = 146;
const uint8_t _X__X__XX = 147;
const uint8_t _X__X_X__ = 148;
const uint8_t _X__X_X_X = 149;
const uint8_t _X__X_XX_ = 150;
const uint8_t _X__X_XXX = 151;
const uint8_t _X__XX___ = 152;
const uint8_t _X__XX__X = 153;
const uint8_t _X__XX_X_ = 154;
const uint8_t _X__XX_XX = 155;
const uint8_t _X__XXX__ = 156;
const uint8_t _X__XXX_X = 157;
const uint8_t _X__XXXX_ = 158;
const uint8_t _X__XXXXX = 159;
const uint8_t _X_X_____ = 160;
const uint8_t _X_X____X = 161;
const uint8_t _X_X___X_ = 162;
const uint8_t _X_X___XX = 163;
const uint8_t _X_X__X__ = 164;
const uint8_t _X_X__X_X = 165;
const uint8_t _X_X__XX_ = 166;
const uint8_t _X_X__XXX = 167;
const uint8_t _X_X_X___ = 168;
const uint8_t _X_X_X__X = 169;
const uint8_t _X_X_X_X_ = 170;
const uint8_t _X_X_X_XX = 171;
const uint8_t _X_X_XX__ = 172;
const uint8_t _X_X_XX_X = 173;
const uint8_t _X_X_XXX_ = 174;
const uint8_t _X_X_XXXX = 175;
const uint8_t _X_XX____ = 176;
const uint8_t _X_XX___X = 177;
const uint8_t _X_XX__X_ = 178;
const uint8_t _X_XX__XX = 179;
const uint8_t _X_XX_X__ = 180;
const uint8_t _X_XX_X_X = 181;
const uint8_t _X_XX_XX_ = 182;
const uint8_t _X_XX_XXX = 183;
const uint8_t _X_XXX___ = 184;
const uint8_t _X_XXX__X = 185;
const uint8_t _X_XXX_X_ = 186;
const uint8_t _X_XXX_XX = 187;
const uint8_t _X_XXXX__ = 188;
const uint8_t _X_XXXX_X = 189;
const uint8_t _X_XXXXX_ = 190;
const uint8_t _X_XXXXXX = 191;
const uint8_t _XX______ = 192;
const uint8_t _XX_____X = 193;
const uint8_t _XX____X_ = 194;
const uint8_t _XX____XX = 195;
const uint8_t _XX___X__ = 196;
const uint8_t _XX___X_X = 197;
const uint8_t _XX___XX_ = 198;
const uint8_t _XX___XXX = 199;
const uint8_t _XX__X___ = 200;
const uint8_t _XX__X__X = 201;
const uint8_t _XX__X_X_ = 202;
const uint8_t _XX__X_XX = 203;
const uint8_t _XX__XX__ = 204;
const uint8_t _XX__XX_X = 205;
const uint8_t _XX__XXX_ = 206;
const uint8_t _XX__XXXX = 207;
const uint8_t _XX_X____ = 208;
const uint8_t _XX_X___X = 209;
const uint8_t _XX_X__X_ = 210;
const uint8_t _XX_X__XX = 211;
const uint8_t _XX_X_X__ = 212;
const uint8_t _XX_X_X_X = 213;
const uint8_t _XX_X_XX_ = 214;
const uint8_t _XX_X_XXX = 215;
const uint8_t _XX_XX___ = 216;
const uint8_t _XX_XX__X = 217;
const uint8_t _XX_XX_X_ = 218;
const uint8_t _XX_XX_XX = 219;
const uint8_t _XX_XXX__ = 220;
const uint8_t _XX_XXX_X = 221;
const uint8_t _XX_XXXX_ = 222;
const uint8_t _XX_XXXXX = 223;
const uint8_t _XXX_____ = 224;
const uint8_t _XXX____X = 225;
const uint8_t _XXX___X_ = 226;
const uint8_t _XXX___XX = 227;
const uint8_t _XXX__X__ = 228;
const uint8_t _XXX__X_X = 229;
const uint8_t _XXX__XX_ = 230;
const uint8_t _XXX__XXX = 231;
const uint8_t _XXX_X___ = 232;
const uint8_t _XXX_X__X = 233;
const uint8_t _XXX_X_X_ = 234;
const uint8_t _XXX_X_XX = 235;
const uint8_t _XXX_XX__ = 236;
const uint8_t _XXX_XX_X = 237;
const uint8_t _XXX_XXX_ = 238;
const uint8_t _XXX_XXXX = 239;
const uint8_t _XXXX____ = 240;
const uint8_t _XXXX___X = 241;
const uint8_t _XXXX__X_ = 242;
const uint8_t _XXXX__XX = 243;
const uint8_t _XXXX_X__ = 244;
const uint8_t _XXXX_X_X = 245;
const uint8_t _XXXX_XX_ = 246;
const uint8_t _XXXX_XXX = 247;
const uint8_t _XXXXX___ = 248;
const uint8_t _XXXXX__X = 249;
const uint8_t _XXXXX_X_ = 250;
const uint8_t _XXXXX_XX = 251;
const uint8_t _XXXXXX__ = 252;
const uint8_t _XXXXXX_X = 253;
const uint8_t _XXXXXXX_ = 254;
const uint8_t _XXXXXXXX = 255;

}

#endif // !defined(_DISPLAY_H_)
