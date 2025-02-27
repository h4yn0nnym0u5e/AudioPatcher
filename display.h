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
    uint16_t modeColour;
    uint16_t* screenBuffer = nullptr;
    struct {int16_t x,y,w,h;} savedArea;
    static const int16_t CURSOR_SIZE = 10;
    static const int16_t OBJECT_SIZE = 48;
    
    void GetCursorSaveParams(const int16_t x, const int16_t y, int16_t& cxr, int16_t& cyr, int16_t& cw, int16_t& ch);
    bool objIsOnScreen(int16_t x, int16_t y, int16_t w = OBJECT_SIZE, int16_t h = OBJECT_SIZE); // call after canvas co-ordinate transformation
    bool screenReadOK(void);
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
    void ShowBottomText(const char* txt, uint16_t colour);
    void ShowStatus(const char* txt,int16_t x,int16_t y, uint16_t colour);
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
    
    // parameter settings box is screeen-relative
    void SaveArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void InitArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void RestoreArea(void);
    void ShowTitle(const char* t, int16_t xoff, int16_t yoff);
    void ShowVoiceFlag(bool flag);
    void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t colour);
    void ShowLabel(const ParamEntry& p, ParamValue& v, int16_t n, int16_t xoff, int16_t yoff);
    void ShowValue(const ParamEntry& p, ParamValue& v, int16_t n);
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

extern AudioPatcherTouch touch;

#endif // !defined(_DISPLAY_H_)
