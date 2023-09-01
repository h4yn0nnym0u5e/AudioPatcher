#if !defined(_DISPLAY_H_)
#define _DISPLAY_H_

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"
#include "objects.h"
#include "paramEntry.h"


#define TFT_DC  9
#define TFT_CS 15
#define TFT_ROTATION 3

#define CONNECTION_COLOUR    0x9492 // ILI9341_DARKGREY
#define PATCHCORD_COLOUR     0xFC02 // orange
#define PATCHCORD_HIGHLIGHT  0xFE04 // orange

//A 320x240x16-bit display takes 153,600 bytes to store
class AudioPatcherDisplay
{
    uint16_t* cbuf = nullptr; 
    int16_t cursor_x; 
    int16_t cursor_y;
    uint16_t modeColour;
    uint16_t* screenBuffer = nullptr;
    struct {int16_t x,y,h,w;} savedArea;
    static const int16_t CURSOR_SIZE = 10;
    
    void GetCursorSaveParams(const int16_t x, const int16_t y, int16_t& cxr, int16_t& cyr, int16_t& cw, int16_t& ch);
  public:  
    void Init(void);
    void Clear(void);
    void Splash(void);
    void DrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y, bool greyed = false);
    void EraseAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y);
    void HighlightAudioObject(int16_t x, int16_t y, bool on = true);
    void HighlightAudioObject(int16_t x, int16_t y, uint16_t colour);
    void DrawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n = 0, bool op = false, uint16_t colour = CONNECTION_COLOUR);
    void DrawPatchcord(AudioObjInstance& src, int8_t sp, AudioObjInstance& dst, int8_t dp, uint16_t colour = PATCHCORD_COLOUR);
    void DrawPatchcord(PatchcordInstance_t* cord, uint16_t colour = PATCHCORD_COLOUR) { DrawPatchcord(*cord->src,cord->src_port,*cord->dst,cord->dst_port, colour); }
    void ShowMode(const char* txt);
    void ShowSelection(const char* txt, AudioCategory_e cat);
    void ShowBottomText(const char* txt, uint16_t colour);
    void CursorSave(void);
    void CursorClear(void);
    void CursorRestore(void);
    void CursorTo(int16_t x, int16_t y);
    void GetCursor(int16_t& x, int16_t& y);
    uint16_t getModeColour(void) { return modeColour; }
    
    void SaveArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void InitArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void RestoreArea(void);
    void ShowTitle(const char* t, int16_t xoff, int16_t yoff);
    void ShowLabel(const ParamEntry& p, ParamValue& v, int16_t n, int16_t xoff, int16_t yoff);
    void ShowValue(const ParamEntry& p, ParamValue& v, int16_t n);
    void ShowStatus(const char* txt,int16_t x,int16_t y, uint16_t colour);
};

extern AudioPatcherDisplay display;

#endif // !defined(_DISPLAY_H_)
