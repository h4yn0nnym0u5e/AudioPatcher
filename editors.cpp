#include "editors.h"

static PatchcordInstance_t blankPatch;
//======================================================================
//======================================================================
AudioObjInstance* BaseEditor::highlightObj(AudioObjInstance* it, uint16_t colour)
{
  if (nullptr != it)
    display.HighlightAudioObject(it->x,it->y,colour);

  return it;
}

AudioObjInstance* BaseEditor::highlightObjnum(int n, uint16_t colour)
{
  AudioObjInstance* it = nullptr;
  
  if (!objVec.empty() && n < (int) objVec.size())
  {
    it = objVec.at(n).p;
    highlightObj(it,colour);
  }

  return it;
}

//======================================================================
void ObjEditor::ShowSelection(int v)
{
    Serial.printf("%d : %s\n",v,objList[v].name);
    display.ShowSelection(objList[v].name,objList[v].category);
}


void ObjEditor::edit(void)
{
  // Scroll through available object types
  if (enc2.available() || !initialised)
  {
    int v = enc2.getValue();
    lastType = v;
    ShowSelection(v);
  }

  // Place an instance of the currently-selected object type
  if (enc2.getButton())
    state = 1;
  else
  {
    if (1 == state) // enc2 released
    {
      int16_t x,y;
      
      state = 0;
      display.GetCursor(x,y);
      objVec.push_back({new AudioObjInstance(objList[enc2.getValue()],x-24,y-24)});
                               
      AudioObjInstance* ao = objVec.back().p;;
      display.CursorClear();
      display.DrawAudioObject(*ao->objP,ao->x,ao->y);
      display.CursorRestore();
      std::stable_sort(objVec.begin(),objVec.end());
    }
  }

  // Move the cross-hair cursor
  if (enc0.available() || enc1.available() || !initialised)
  {
    int16_t x = enc0.getValue() * 10;
    int16_t y = enc1.getValue() * 10;
    display.CursorTo(x,y);
  } 

  if (!initialised)
    initialised = true;
}


//----------------------------------------------------------------------
void ObjEditor::enter(void)
{
  enc0.setLimits(0,31);
  enc2.setLimits(1,COUNT_OF_objList);
  enc2.setValue(lastType);
  ShowSelection(enc2.getValue());
}



//======================================================================
//======================================================================
void CordEditor::ShowSelection(int io)
{
  char buf[5];
  
  sprintf(buf,"%s",io?"dst":"src");
  display.ShowSelection(buf,AudioCategory_patchcord);
  greyOut(io?noDst:noSrc);
}

//----------------------------------------------------------------------
void CordEditor::highlightPort(AudioObjInstance* aoi, int io, int n, bool on)
{
  uint16_t colour = on?PATCHCORD_COLOUR:CONNECTION_COLOUR;
  if (nullptr != aoi)
  {
    display.DrawConnection(*aoi->objP,aoi->x,aoi->y,n,0 == io,colour);
    // Serial.printf("highlight %s.%d: %s\n",io?"input":"output",n,on?"on":"off");
  }
}

//----------------------------------------------------------------------
int CordEditor::findGoodObj(int epIdx, int ec1, int io)
{
  AudioObjInstance* aoi;
  int dir = ec1 > epIdx 
                ? 1
                :(ec1 < epIdx ? -1 : 0);

  bool ok = false;
  while (!ok)
  {
    if (ec1 < 0 || ec1 >= (int) objVec.size()) // off end?
      break; // give up
      
    aoi = objVec.at(ec1).p;
    
    if (1 == io)  // we're setting destination...
    {
      if (aoi->inputAvailFlags > 0) // ...and have inputs available
        ok = true;
    }
    else // we're setting source
    {
      if (aoi->objP->outputs > 0) // object has outputs
        ok = true;
    }

    if (!ok) // this object won't do
    {
      if (0 == dir) // selection can't be changed
        break;
      ec1 += dir; // move on or back
    }
  }

  if (!ok)
    ec1 = -1;

  return ec1;    
}

int CordEditor::findGoodPort(int epIdx, int portNum, int io)
{
  int port = -1; // assume we'll fail 
  AudioObjInstance* aoi = objVec.at(epIdx).p;

  do
  {
    if (1 == io)  // we're setting destination...
    {
      if (0 == aoi->inputAvailFlags) // nothing left, give up
        break;
        
      if (aoi->inputAvailFlags & (1<<portNum)) // already OK
      {
        port = portNum;
        break;
      }

      port = 0; 
      uint32_t flag = 1;
      while (flag != 0                           // still some to check
        && (0 == (flag & aoi->inputAvailFlags))) // but not this one
      {
        port++;
        flag <<= 1;
      }
      if (0 != (flag & aoi->inputAvailFlags)) // found one!
        break;
    }
    else // we're setting source
    {
      if (0 == aoi->objP->outputs) // nothing here, fail
        break;

      if (portNum < aoi->objP->outputs) // it's already valid
      {
        port = portNum;
        if (portNum < 0)
          port = 0;
        break;
      }
      else
      {
        port = aoi->objP->outputs - 1;
        break;
      }      
    }
    port = -1;
  } while (0);

  return port;
}


int CordEditor::findBestSettings(settings& ns, Prioritise pri)
{
  static int depth = 0;
  int result = 0;

  depth++;
  // pri actually tells us what's just been changed: if it's
  // nothing, then we're trying to re-validate the provided settings
  switch (pri)
  {
    default:
    case Prioritise::object: // object changed, prioritise that
      {
        int newObj;

        do
        {
          newObj = findGoodObj(epIdx,ns.objNum,ns.srcdst);
          if (newObj >= 0) break; // this is an object which we can use
          
          if (epIdx == ns.objNum)
          {
            newObj = findGoodObj(epIdx,epIdx+1,ns.srcdst);
            if (newObj >= 0) break; // this is an object which we can use
          }
          
          newObj = findGoodObj(epIdx,2*epIdx - ns.objNum,ns.srcdst); // search the other way
        } while (0);
        
        if (newObj >= 0) // this is an object which we can use...
        {
          ns.objNum = newObj; // ...use it ...
          result = findBestSettings(ns,Prioritise::port); // ...and find the best port, too
        }
        else 
        {
          if (depth < 2)
          {
            ns.srcdst = 1-ns.srcdst;
            result = findBestSettings(ns,Prioritise::object);
            if (result < 0) // no luck
              ns.srcdst = 1-ns.srcdst;
          }
          else
            result = -1; // can't do anything          
        }
      }
      break;
      
    case Prioritise::port: // port changed, prioritise that
      {
        int newPort = findGoodPort(ns.objNum,ns.portNum,ns.srcdst);
        if (newPort >= 0)
          ns.portNum = newPort;
        else
          result = -1;
      }
      break;
      
    case Prioritise::srcdst: // src/dst changed, prioritise that
      {
        int newObj = findGoodObj(ns.objNum,ns.objNum,ns.srcdst); // current object OK?
        if (newObj < 0) // nope
        {
          ns.objNum = epIdx + 1;
          result = findBestSettings(ns,Prioritise::object); // try to find a workable object
        }
        else
        {
          ns.objNum = newObj; // ...use it ...
          result = findBestSettings(ns,Prioritise::port); // ...and find the best port, too          
        }        
      }
      break;
  }

  depth--;
  return result;
}


void CordEditor::edit(void)
{
  bool changedIdx = false;
  int io = 1-enc2.getValue(); // i = 1 = input = dst, o = 0 = output = src
  CordEditor::settings newSettings = {epIdx,portNum,io};
  bool redrawSelected = false;  
  
  //-----------------------------------------------
  // select an audio object
  if (enc0.available())
  {
    int tmp = newSettings.objNum = enc0.getValue();
    findBestSettings(newSettings,Prioritise::object);
    if ((tmp > epIdx) == (newSettings.objNum > epIdx))
      redrawSelected = true;
    else
    {
      enc0.setValue(epIdx);
      newSettings = {epIdx,portNum,io}; 
    }
  }

  // select a connection port
  if (changedIdx || enc1.available())
  {
    int tmp = newSettings.portNum = enc1.getValue();
    findBestSettings(newSettings,Prioritise::port);
    if ((tmp > portNum) == (newSettings.portNum > portNum))
      redrawSelected = true;
    else
    {
      enc1.setValue(portNum);
      newSettings = {epIdx,portNum,io}; 
    }
  }
  
  if (enc2.available()) // src / dst switch
  {
    newSettings.srcdst = 1 - enc2.getValue();
    findBestSettings(newSettings,Prioritise::srcdst);
    redrawSelected = true;
  }

  //-----------------------------------------------
  AudioObjInstance* aoi = objVec.at(epIdx).p;
  
  if (portNum != newSettings.portNum || epIdx != newSettings.objNum)
  {
    highlightPort(aoi,io,portNum,false); 
    portNum = newSettings.portNum+1; // force port highlight later
  }
    
  if (epIdx != newSettings.objNum)
  {
    highlightObjnum(epIdx,ILI9341_BLACK);
    epIdx = newSettings.objNum;        
    aoi = highlightObjnum(epIdx,ILI9341_WHITE);
    enc0.setValue(epIdx); // in case we skipped some
  }
  
  if (io != newSettings.srcdst)
  {
    io = newSettings.srcdst;
    ShowSelection(io);
    enc2.setValue(1-io);
    portNum = newSettings.portNum+1; // force port highlight later
  }
  
  if (portNum != newSettings.portNum)
  {
    portNum = newSettings.portNum;
    highlightPort(aoi,io,portNum,true); 
    enc1.setValue(portNum); // in case we skipped some
  }

  if (redrawSelected)
  {
    if (nullptr != editCord.src)
      highlightPort(editCord.src,0,editCord.src_port,true); 
    if (nullptr != editCord.dst)
      highlightPort(editCord.dst,1,editCord.dst_port,true); 
  }
  
  //-----------------------------------------------
  // select a source or destination
  if (enc1.getButton())
    state = 1;
  else
  {
    if (1 == state)
    {
      AudioObjInstance* aoi = objVec.at(epIdx).p;
      
      state = 0;
      if (1 == io) // set dst / input
      {
        highlightPort(editCord.dst,1,editCord.dst_port,false);
        if (editCord.dst == aoi)
        {
          //Serial.println("dst cleared");
          editCord.dst = nullptr;
        }
        else
        {
          //Serial.println("dst set");
          highlightObj(editCord.dst,ILI9341_BLACK);
          editCord.dst = aoi;
          editCord.dst_port = portNum;
          highlightPort(editCord.dst,1,editCord.dst_port,true);
        }      
      }
      else // set src / output
      {
        highlightPort(editCord.src,0,editCord.src_port,false);
        if (editCord.src == aoi)
        {
          //Serial.println("src cleared");
          editCord.src = nullptr;        
        }
        else
        {
          //Serial.println("src set");
          highlightObj(editCord.src,ILI9341_BLACK);
          editCord.src = aoi;
          editCord.src_port = portNum;      
          highlightPort(editCord.src,0,editCord.src_port,true);
        }
      }

      if (nullptr != editCord.src && nullptr != editCord.dst) // both set - create patchcord
      {
          cordVec.push_back(new PatchcordInstance_t{editCord.src,editCord.src_port,editCord.dst,editCord.dst_port}); // create
          display.DrawPatchcord(cordVec.back()); // draw
          highlightPort(editCord.src,0,editCord.src_port,false); // un-highlight...
          highlightPort(editCord.dst,1,editCord.dst_port,false); // ...the ports...
          if (aoi == editCord.src)
            highlightObj(editCord.dst,ILI9341_BLACK); // ...and the other...
          if (aoi == editCord.dst)
            highlightObj(editCord.src,ILI9341_BLACK); // ...audio object!
            
          editCord = blankPatch;

          int ec1;
          do
          {
            ec1 = findGoodObj(epIdx,epIdx,io); // can we stay on this object?
            if (ec1 >= 0) // yes
              break;
              
            ec1 = findGoodObj(epIdx,epIdx+1,io); // can we find a later one?
            if (ec1 >= 0) // yes
              break;
              
            ec1 = findGoodObj(epIdx,epIdx-1,io); // can we find an earlier one?
            if (ec1 >= 0) // yes
              break;

            io = 1-io; // try swapping src <-> dst               
              
            ec1 = findGoodObj(epIdx,epIdx,io); // can we stay on this object?
            if (ec1 >= 0) // yes
              break;
              
            ec1 = findGoodObj(epIdx,epIdx+1,io); // can we find a later one?
            if (ec1 >= 0) // yes
              break;
              
            ec1 = findGoodObj(epIdx,epIdx-1,io); // can we find an earlier one?
            if (ec1 >= 0) // yes
              break;
              
          } while (0);

          if (ec1 >= 0 && (ec1 != epIdx || io != enc2.getValue()))
          {
            if (io != enc2.getValue()) // had to swap src <-> dst
            {
              ShowSelection(io);
              enc2.setValue(1-io);
            }

            if (ec1 != epIdx)
            {
              enc0.setValue(ec1);
              highlightObjnum(epIdx,ILI9341_BLACK);
              highlightObjnum(ec1,ILI9341_WHITE);
            }
          }
      }
    }
  }
}

void CordEditor::enter(void)
{
  AudioObjInstance* aoi;
  
  enc0.setValue(epIdx);
  enc0.setLimits(0,objVec.size() -1);
  
  enc2.setLimits(0,1);
  enc2.setValue(1); 
  
  ShowSelection(0);
  aoi = highlightObjnum(epIdx,ILI9341_WHITE);
  editCord = blankPatch;

  portNum = 0;
  highlightPort(aoi,0,portNum,true);
}

void CordEditor::exit(void)
{
  highlightObjnum(epIdx,ILI9341_BLACK);
  greyOut(nothing);
}

void CordEditor::greyOut(srctype s)
{
  for (auto obj : objVec)
  {
    bool grey = false;
    if (s == noSrc && 0 == obj.p->objP->outputs) grey = true;
    if (s == noDst && (0 == obj.p->objP->inputs || 0 == obj.p->inputAvailFlags))  grey = true;
    display.DrawAudioObject(*obj.p->objP,obj.p->x,obj.p->y,grey);
  }
}


//======================================================================
//======================================================================
void DeleteEditor::enter(void)
{
  delType = delObj;
  
  enc0.setValue(epIdx);
  enc0.setLimits(0,objVec.size() -1);
  highlight(-1,epIdx);
  
  enc2.setLimits(0,1);
  enc2.setValue(delType);
  ShowSelection(enc2.getValue());  
}

void DeleteEditor::exit(void)
{
  highlight(epIdx,-1);  
}
void DeleteEditor::edit(void)
{
  // select an audio object
  if (enc0.available())
  {
    int ec1 = enc0.getValue();
    highlight(epIdx,ec1);
    epIdx = ec1;
  }
  
  if (enc2.available())
  {
    highlight(epIdx,-1);
    delType = (delType_e) enc2.getValue();
    ShowSelection(delType);
    enc0.setLimits(0,delType == delObj
                              ?(objVec.size() - 1)
                              :(cordVec.size() - 1));
    epIdx = enc0.getValue();
    highlight(-1,epIdx);
  }

  if (enc0.getButton())
    state = 1;
  else
  {
    if (1 == state)
    {
      state = 0;
      switch (delType) // time to delete a thing!
      {
        case delObj:
          {
            AudioObjInstance* aoi = objVec.at(epIdx).p;

            if (!aoi->noDelete) // I/O and control are protected
            {
              // delete associated patchcords: go backwards
              // so we don't change the index of cords we
              // haven't checked yet
              for (int i=cordVec.size() - 1;i>=0;--i)
              {
                auto cord = cordVec.at(i);
                if (cord->src == aoi || cord->dst == aoi)
                {
                  display.DrawPatchcord(cord,ILI9341_BLACK);
                  delete cord;
                  cordVec.erase(std::next(cordVec.begin(),i));
                }
              }
  
              // now we can delete the audio object
              display.EraseAudioObject(*aoi->objP,aoi->x,aoi->y); // from the display...
              objVec.erase(std::next(objVec.begin(),epIdx));
            }
          }
          break;

        case delCord:
          {
            PatchcordInstance_t* ppc = cordVec.at(epIdx);
            display.DrawPatchcord(ppc,ILI9341_BLACK);
            delete ppc;
            cordVec.erase(std::next(cordVec.begin(),epIdx));
            enc0.setLimits(0,cordVec.size()-1);
          }
          break;
      }

      // selection has changed, update display
      epIdx = enc0.getValue();
      highlight(-1,epIdx);
    }
  }
}


void DeleteEditor::highlight(int remove, int add)
{
  switch (delType)
  {
    case delObj:
    
      if (remove >= 0)
      {
        AudioObjInstance* aoi = 
          highlightObjnum(remove,ILI9341_BLACK); // take highlight off old object
        for (auto cord : cordVec) // highlight attached patchcords, too
          if (cord->src == aoi || cord->dst == aoi)
            display.DrawPatchcord(cord);                 
      }
      if (add >= 0)
      {
        AudioObjInstance* aoi = objVec.at(add).p;
        uint16_t colour = aoi->noDelete?ILI9341_RED:ILI9341_WHITE;
        
        highlightObjnum(add,colour); // put it on the new one
        if (!aoi->noDelete) // only if deletable
        {
          for (auto cord : cordVec) // highlight attached patchcords, too
            if (cord->src == aoi || cord->dst == aoi)
              display.DrawPatchcord(cord,PATCHCORD_HIGHLIGHT);                
        }
      }
      break;

    case delCord:
      if (remove >= 0)
        display.DrawPatchcord(cordVec.at(remove)); // take highlight off old object
      if (add >= 0)
        display.DrawPatchcord(cordVec.at(add),PATCHCORD_HIGHLIGHT); // put it on the new one     
      break;
  }
}

void DeleteEditor::ShowSelection(int op)
{
  char buf[20];
  
  sprintf(buf,"%s",op?"patchcords":"objects");
  display.ShowBottomText(buf,display.getModeColour());
}


//======================================================================
//======================================================================
void FileEditor::enter(void)
{
}

void FileEditor::exit(void)
{
}

void FileEditor::edit(void)
{
  if (enc0.getButton())
    state = 1;
  else
  {
    if (state)
    {
      state = 0;
      
      Serial.println("\nSave the patch:");
      for (auto obj : objVec)
        Serial.printf("#%d: %d @ %d,%d\n",state++,obj.p->objP->id,obj.p->x,obj.p->y);
      state = 0;
      for (auto obj : ioVec)
        Serial.printf("#%d: %d @ %d,%d\n",--state,obj.p->objP->id,obj.p->x,obj.p->y);
      for (auto cord : cordVec)
      {
        const int UNSET = 999'999'999;
        int src = UNSET,dst = UNSET;
        
        state = 0;
        for (auto obj : objVec)
        {
          if (obj.p == cord->src) src = state;
          if (obj.p == cord->dst) dst = state;
          if (src != UNSET && dst != UNSET)
            break;
          state++;            
        }

        if (src == UNSET || dst == UNSET)
        {
          state = 0;
          for (auto obj : ioVec)
          {
            --state;
            if (obj.p == cord->src) src = state;
            if (obj.p == cord->dst) dst = state;
            if (src != UNSET && dst != UNSET)
              break;
          }
        }
        
        Serial.printf("%d.%d -> %d.%d\n",src,cord->src_port,dst,cord->dst_port);
      }
      Serial.println("---------------\n");
        
      state = 0;
    }
  }
}
