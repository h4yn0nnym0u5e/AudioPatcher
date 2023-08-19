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
      if (ec1 < 0 || ec1 >= (int) objVec.size()) // off end?
        break; // give up
    }
  }

  if (!ok)
    ec1 = -1;

  return ec1;    
}


void CordEditor::edit(void)
{
  bool changedIdx = false;
  int io = 1-enc2.getValue(); // i = 1 = input = dst, o = 0 = output = src
  
  // select an audio object
  if (enc0.available())
  {
    int ec1 = enc0.getValue();

    do
    {
      AudioObjInstance* aoi;
      int dir = ec1 > epIdx 
                    ? 1
                    :(ec1 < epIdx ? -1 : 0);
      if (0 == dir) // selection has not changed
        break;

      bool ok = false;
      while (!ok)
      {
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
          ec1 += dir; // move on or back
          if (ec1 < 0 || ec1 >= (int) objVec.size()) // off end?
            break; // give up
        }
      }

      if (!ok) // can't go that way
      {
        enc0.setValue(epIdx); // stay where we were
        break;
      }
      else
        enc0.setValue(ec1); // ok, but might have changed

      aoi = objVec.at(epIdx).p;
  
      if (aoi != editCord.src && aoi != editCord.dst)
      {
        highlightObjnum(epIdx,ILI9341_BLACK);
        highlightPort(aoi,io,portNum,false);
      }
      else
        highlightObjnum(epIdx,PATCHCORD_COLOUR);
      
      epIdx = ec1;
      aoi = highlightObjnum(epIdx,ILI9341_WHITE);

      // find any output, or available input
      portNum = 0;
      if (aoi == editCord.src) portNum = editCord.src_port;
      if (aoi == editCord.dst) portNum = editCord.dst_port;
      uint32_t flag = 1;
      while (1 == io 
          && 0 != aoi->inputAvailFlags // double-check: ought to be true
          && 0 == (aoi->inputAvailFlags & flag))
      {
        portNum++;
        flag <<= 1;
      }
      
      enc1.setValue(portNum);
      changedIdx = true;
    } while (0);
  }

  // select a connection port
  if (changedIdx || enc1.available())
  {
    int newPortNum = enc1.getValue();
    AudioObjInstance* aoi = objVec.at(epIdx).p;
    int numPorts = io?aoi->objP->inputs:aoi->objP->outputs;

    if (!changedIdx)
      highlightPort(aoi,io,portNum,false);
    if (0 == numPorts)
    {
      io = 1-io;
      enc2.setValue(1-io);
      ShowSelection(io);
      numPorts = io?aoi->objP->inputs:aoi->objP->outputs;
      changedIdx = true;
    }
    
    if (newPortNum >= numPorts)
    {
      newPortNum = numPorts - 1; // always >= 0
    }

    portNum = newPortNum;
    enc1.setValue(portNum);
    highlightPort(aoi,io,portNum,true);
  }
  
  if (enc2.available()) // src / dst switch
  {
    AudioObjInstance* aoi = objVec.at(epIdx).p;
    highlightPort(aoi,io,portNum,false);
    
    io = 1-enc2.getValue();
    ShowSelection(io);
    
    if (nullptr != editCord.src)
      highlightPort(editCord.src,0,editCord.src_port,true);
    if (nullptr != editCord.dst)
      highlightPort(editCord.dst,1,editCord.dst_port,true);
      
    portNum = 0;
    enc1.setValue(portNum);
    highlightPort(aoi,io,portNum,true);
  }

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
        AudioObjInstance* aoi = 
          highlightObjnum(add,ILI9341_WHITE); // put it on the new one
        for (auto cord : cordVec) // highlight attached patchcords, too
          if (cord->src == aoi || cord->dst == aoi)
            display.DrawPatchcord(cord,PATCHCORD_HIGHLIGHT);                
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
