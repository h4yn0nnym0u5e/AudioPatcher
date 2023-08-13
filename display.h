#if !defined(_DISPLAY_H_)
#define _DISPLAY_H_

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"
#include "objects.h"

#define TFT_DC  9
#define TFT_CS 10
#define TFT_ROTATION 3

#define CONNECTION_COLOUR 0x9492 //ILI9341_DARKGREY


extern void displayInit(void);
extern void displayClear(void);
extern void displaySplash(void);
extern void displayDrawAudioObject(AudioObjStatic_t& o, int16_t x, int16_t y);
extern void drawConnection(AudioObjStatic_t& o, int16_t x, int16_t y, int8_t n = 0, bool op = false, uint16_t colour = CONNECTION_COLOUR);

#endif // !defined(_DISPLAY_H_)
