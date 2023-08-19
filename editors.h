#if !defined(_EDITORS_H_)
#define _EDITORS_H_

#include "display.h"
#include "limitedEncoder.h"
#include "objects.h"
#include <vector>
#include <algorithm>

class BaseEditor
{
  protected:
    AudioPatcherDisplay& display;
    std::vector<AudioObjInstancePtr>& objVec;
    int epIdx;
    int state;

  public:
    BaseEditor(AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o)
            : display(d), objVec(o), epIdx(0), state(0)
            {}
    AudioObjInstance* highlightObjnum(int n, uint16_t colour);
    AudioObjInstance* highlightObj(AudioObjInstance* it, uint16_t colour); 
};

class ObjEditor : public BaseEditor
{
    LimitedEncoder& enc0, &enc1, &enc2;
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
            : BaseEditor(d,o), 
             enc0(e0), enc1(e1), enc2(e2),
             objList(ol),
            state(0), initialised(false)
            {}
    void edit(void);
    void enter(void);
    void exit(void) {}
};


class CordEditor : public BaseEditor
{
    enum srctype {nothing,noSrc,noDst};
    enum class Prioritise {nothing,object,port,srcdst};
    struct settings {int objNum, portNum, srcdst;};

    LimitedEncoder& enc0, &enc1, &enc2;
    std::vector<PatchcordInstance_t*>& cordVec; 
    AudioObjStatic_t (&objList)[];
    int portNum;
    PatchcordInstance_t editCord;

    int findBestSettings(settings& ns, Prioritise pri);
    int findGoodPort(int epIdx, int portNum, int io); 
    int findGoodObj(int epIdx, int ec1, int io);
    void ShowSelection(int io);
    void highlightPort(AudioObjInstance* aoi, int io, int n, bool on);
    void greyOut(srctype s);

  public:
    CordEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p,
            AudioObjStatic_t (&ol)[])
            : BaseEditor(d,o),
            enc0(e0), enc1(e1), enc2(e2),
            cordVec(p),
            objList(ol),
            portNum(0)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 
};

class DeleteEditor : public BaseEditor
{
    enum delType_e {delObj,delCord} delType;
    enum highType {both, add, remove};
    
    LimitedEncoder& enc0, &enc1, &enc2;
    std::vector<PatchcordInstance_t*>& cordVec; 
    void ShowSelection(int io);
    void highlight(int remove, int add);
     
  public:    
    DeleteEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p
            )
            : BaseEditor(d,o),
            enc0(e0), enc1(e1), enc2(e2),
            cordVec(p)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 
};


class FileEditor : public BaseEditor
{
    LimitedEncoder& enc0, &enc1, &enc2;
    std::vector<AudioObjInstancePtr>& ioVec;
    std::vector<PatchcordInstance_t*>& cordVec; 
    int state;
     
  public:    
    FileEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<AudioObjInstancePtr>& io,
            std::vector<PatchcordInstance_t*>& p
            )
            : BaseEditor(d,o),
            enc0(e0), enc1(e1), enc2(e2),
            ioVec(io), cordVec(p),
            state(0)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 
};

#endif // !defined(_EDITORS_H_)
