#if !defined(_EDITORS_H_)
#define _EDITORS_H_

#include "display.h"
#include "limitedEncoder.h"
#include "objects.h"
#include <vector>
#include <algorithm>

class ObjEditor 
{
    LimitedEncoder& enc0, &enc1, &enc2;
    AudioPatcherDisplay& display;
    std::vector<AudioObjInstancePtr>& objVec;
    AudioObjStatic_t (&objList)[];
    int state;
    bool initialised;
    int lastType;

    void ShowSelection(int v);
  public:
    ObjEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            AudioObjStatic_t (&ol)[])
            : enc0(e0), enc1(e1), enc2(e2),
            display(d), objVec(o), objList(ol),
            state(0), initialised(false)
            {}
    void edit(void);
    void enter(void);
    void exit(void) {}
};


class CordEditor 
{
    enum srctype {nothing,noSrc,noDst};

    LimitedEncoder& enc0, &enc1, &enc2;
    AudioPatcherDisplay& display;
    std::vector<AudioObjInstancePtr>& objVec;
    AudioObjStatic_t (&objList)[];
    int epIdx,portNum;
    PatchcordInstance_t editCord;
    int state;

    void ShowSelection(int io);
    AudioObjInstance* highlightObjnum(int n, uint16_t colour);
    AudioObjInstance* highlightObj(AudioObjInstance* it, uint16_t colour);
    void highlightPort(AudioObjInstance* aoi, int io, int n, bool on);
    void greyOut(srctype s);

  public:
    CordEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            AudioObjStatic_t (&ol)[])
            : enc0(e0), enc1(e1), enc2(e2),
            display(d), objVec(o), objList(ol),
            epIdx(0), portNum(0), state(0)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 
};

#endif // !defined(_EDITORS_H_)
