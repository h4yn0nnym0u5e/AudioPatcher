#if !defined(_DISPLAY_H_)
#define _DISPLAY_H_

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"
#include "objects.h"

#define TFT_DC  9
#define TFT_CS 15
#define TFT_ROTATION 3

#define CONNECTION_COLOUR 0x9492 //ILI9341_DARKGREY

class AudioPatcherDisplay
{
    uint16_t* cbuf = nullptr; 
    int16_t cursor_x; 
    int16_t cursor_y;
    static const int16_t CURSOR_SIZE = 10;
    void GetCursorSaveParams(const int16_t x, const int16_t y, int16_t& cxr, int16_t& cyr, int16_t& cw, int16_t& ch);
  public:  
    void Init(void);
    void Clear(void);
    void Splash(void);
    void DrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y);
    void DrawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n = 0, bool op = false, uint16_t colour = CONNECTION_COLOUR);
    void ShowSelection(const char* txt, AudioCategory_e cat);
    void CursorSave(void);
    void CursorClear(void);
    void CursorRestore(void);
    void CursorTo(int16_t x, int16_t y);
    void GetCursor(int16_t& x, int16_t& y);
};

extern AudioPatcherDisplay display;

#endif // !defined(_DISPLAY_H_)
