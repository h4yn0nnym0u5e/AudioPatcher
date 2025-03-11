#if !defined(_EDITORS_H_)
#define _EDITORS_H_

#include "display.h"
#include "limitedEncoder.h"
#include "objects.h"
#include <vector>
#include <algorithm>

extern void lockModeEncoder(void);
extern void unlockModeEncoder(void);
extern void makeFFP(char* buf, const char* base, const char* path, const char* leaf, const char* ext);

#define OBJ_PER_VOICE_CHAR '+'
#define PATCH_ROOT "/patches"

class BaseEditor
{
  protected:
    AudioPatcherDisplay& display;
    std::vector<AudioObjInstancePtr>& objVec;
    std::vector<PatchcordInstance_t*>& cordVec;
    int epIdx;
    int state;
    AudioPatcherDisplay::side where;
    const int16_t CURSOR_STEP=10;
    const int16_t CANVAS_STEP=50;
    const int16_t CORD_SELECT_MIN = 15;
    static const int32_t NO_FORCE=0xCAFEBABE;

  public:
    BaseEditor(AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p)
            : display(d.getInstance()), objVec(o), cordVec(p), epIdx(0), state(0)
            {}
    AudioObjInstance* highlightObjnum(int n, uint16_t colour);
    AudioObjInstance* highlightObj(AudioObjInstance* it, uint16_t colour); 
    int PointToObject(int x, int y);
    int getSide(void) { return (int) where; }
    int PointToCord(int x, int y);
    void SelectByEncoder(LimitedEncoder& enc0, int32_t value=NO_FORCE);
    int SelectByTouch(LimitedEncoder& enc0, bool onlySetEncoder=false);
    int SelectCordByTouch(LimitedEncoder& enc0, bool onlySetEncoder=false);
    void drawAll(bool drawCords = true);
};

class ObjEditor : public BaseEditor
{
    LimitedEncoder& enc0, &enc1, &enc2;
    AudioObjStatic_t (&objList)[];
    int state;
    bool initialised;
    int lastType;
    int16_t xmax,ymax; // display limits

    void ShowSelection(int v);
  public:
    ObjEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p,
            AudioObjStatic_t (&ol)[])
            : BaseEditor(d,o,p), 
             enc0(e0), enc1(e1), enc2(e2),
             objList(ol),
            state(0), initialised(false)
            {}
    void edit(void);
    void enter(void);
    void exit(void) {}

    void create(int id, int x, int y);
};


class CordEditor : public BaseEditor
{
    enum srctype {nothing,noSrc,noDst,touchNothing};
    enum class Prioritise {nothing,object,port,srcdst};
    struct settings {int objNum, portNum, srcdst;};

    LimitedEncoder& enc0, &enc1, &enc2;
    AudioObjStatic_t (&objList)[];
    int portNum;
    settings touchedObj;
    bool setByTouch;
    PatchcordInstance_t editCord;

    int findBestSettings(settings& ns, Prioritise pri);
    int findGoodPort(int epIdx, int portNum, int io); 
    int findGoodObj(int epIdx, int ec1, int io);
    void ShowSelection(int io);
    void highlightPort(AudioObjInstance* aoi, int io, int n, bool on);
    void highlightPort(int epIdx, int io, int n, bool on);
    void greyOut(srctype s);

  public:
    CordEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p,
            AudioObjStatic_t (&ol)[])
            : BaseEditor(d,o,p),
            enc0(e0), enc1(e1), enc2(e2),
            objList(ol),
            portNum(0), setByTouch{false}
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
    void ShowSelection(int io);
    void highlight(int remove, int add);
     
  public:    
    DeleteEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p
            )
            : BaseEditor(d,o,p),
            enc0(e0), enc1(e1), enc2(e2)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 

    void kill(int idx);
};

class ParamEditor : public BaseEditor
{
    LimitedEncoder& enc0, &enc1, &enc2; // we only select objects to modify
    bool inTarget; // true if target object's editor is active
    LimitedEncoderStash* enc0Stash, *enc1Stash, *enc2Stash,
                         *enc0Stash2;
    
  public:    
    ParamEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2,  
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p
            )
            : BaseEditor(d,o,p),
            enc0(e0), enc1(e1), enc2(e2), inTarget(false), 
            enc0Stash(nullptr), enc1Stash(nullptr), enc2Stash(nullptr)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 
};


class MIDIEditor : public BaseEditor
{
    LimitedEncoder& enc0; // we only select objects to modify
    bool inTarget; // true if target object's editor is active
    LimitedEncoderStash* enc0Stash;
    
  public:    
    MIDIEditor(LimitedEncoder& e0,  
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p
            )
            : BaseEditor(d,o,p),
            enc0(e0), inTarget(false), enc0Stash(nullptr)
            {}
    void edit(void);
    void enter(void);
    void exit(void); 
};

class FileListEntry
{
  public:
    FileListEntry(String nm, bool dr) : name{nm}, isDir{dr} {}
    String name;
    bool isDir;
    friend bool operator<(const FileListEntry& lhs, const FileListEntry& rhs)
    {
      if (lhs.isDir && !rhs.isDir) return true;
      return lhs.name < rhs.name;
    }
};

class FileBase
{
  public:
    enum class mode_e {load,save,del};
  protected:   
    AudioPatcherDisplay& fileDisplay; 
    // file names
    static const int MAX_FILE_NAME = 15;
    static const int MAX_FILE_PATH = 42;
    static const char NAME_EOL = '\r';

    // file selection window
    static const int MAX_FILE_LINE =  6;
    static const int FILE_X_OFF =  5;
    static const int FILE_Y_OFF = 27;
    
    LimitedEncoder& enc0, &enc1, &enc2;
    int state, idx;
    std::vector<FileListEntry> fileList;
    char fileName[MAX_FILE_NAME+1];
    char filePath[MAX_FILE_PATH+1];
    const char* basePath;
    size_t basePathLen;
    bool keyboardVisible, upperCase;

    mode_e mode, maxMode;
    void showMode(bool zapCurrent = true);
    virtual void save(const char* nme) {};
    virtual void load(const char* nme) {};
    virtual void del(const char* nme) {};
    virtual void dump(const char* nme) {};
    
    int getLast(char* buf, int maxn);
    void setLast(const char* nme);

    void createFileList(const char* path, mode_e mode);
    void clearFileList(void);
    void showFileList(const int item, bool showAll = false);
    int fileListTop, fileListCurrent;
     
  public:    
    FileBase(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            const char* bp,
            mode_e m
            )
            : fileDisplay(d.getInstance()),
            enc0(e0), enc1(e1), enc2(e2),
            state(0), idx(-1), 
            fileName{0}, basePath{bp},
            maxMode(m)
            {
              basePathLen = strlen(basePath);
            }
    void edit(void);
    void enter(bool saveArea = true);
    void exit(void); 
    
    int loadLast(void);
    void newKey(AudioPatcherDisplay::keyInfo key);
};

class FileEditor : public BaseEditor, public FileBase
{
  public:    
    FileEditor(LimitedEncoder& e0, LimitedEncoder& e1, LimitedEncoder& e2, 
            AudioPatcherDisplay& d,
            std::vector<AudioObjInstancePtr>& o,
            std::vector<PatchcordInstance_t*>& p,
            const char* bp,
            mode_e m
            )
            : BaseEditor(d,o,p), FileBase(e0,e1,e2,d,bp,m)
            {}
    void save(const char* nme);
    void load(const char* nme);
    void del(const char* nme);
    void dump(const char* nme);
};

#endif // !defined(_EDITORS_H_)
