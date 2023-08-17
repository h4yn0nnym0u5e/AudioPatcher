#include "editors.h"

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

void CordEditor::highlightObjnum(int n, uint16_t colour)
{
  if (!objVec.empty() && n < (int) objVec.size())
  {
    auto it = objVec.at(n).p;
    display.HighlightAudioObject(it->x,it->y,colour);
  }  
}

//----------------------------------------------------------------------
void CordEditor::highlightPort(AudioObjInstance* aoi, int io, int n, bool on)
{
  uint16_t colour = on?PATCHCORD_COLOUR:CONNECTION_COLOUR;
  display.DrawConnection(*aoi->objP,aoi->x,aoi->y,n,0 == io,colour);
}

//----------------------------------------------------------------------
void CordEditor::edit(void)
{
  bool changedIdx = false;
  int io = 1-enc2.getValue(); // i = 1 = input = dst, o = 0 = output = src
  
  // select an audio object
  if (enc0.available())
  {
    int ec1 = enc0.getValue();
    AudioObjInstance* aoi = objVec.at(epIdx).p;

    if (aoi != editCord.src && aoi != editCord.dst)
    {
      highlightObjnum(epIdx,ILI9341_BLACK);
      highlightPort(aoi,io,portNum,false);
    }
    else
      highlightObjnum(epIdx,PATCHCORD_COLOUR);
    
    if (ec1 >= 0 && ec1 < (int) objVec.size())
      epIdx = ec1;
    else
      enc0.setValue(epIdx);
    highlightObjnum(epIdx,ILI9341_WHITE);
    
    changedIdx = true;
  }

  // select a connection port
  if (changedIdx || enc1.available())
  {
    int newPortNum = enc1.getValue();
    AudioObjInstance* aoi = objVec.at(epIdx).p;
    int numPorts = io?aoi->objP->inputs:aoi->objP->outputs;

    highlightPort(aoi,io,portNum,false);
    if (0 == numPorts)
    {
      io = 1-io;
      enc2.setValue(1-io);
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
  
  if (changedIdx || enc2.available()) // force an update
  {
    AudioObjInstance* aoi = objVec.at(epIdx).p;
    highlightPort(aoi,io,portNum,false);
    
    io = 1-enc2.getValue();
    ShowSelection(io);
  }

  if (enc2.getButton())
  {
    if (1 == io) // set dst / input
    {
      if (editCord.dst == objVec.at(epIdx).p)
        editCord.dst = nullptr;        
      else
      {
        editCord.dst = objVec.at(epIdx).p;
        editCord.dst_port = portNum;
      }      
    }
    else // set src / output
    {
      if (editCord.src == objVec.at(epIdx).p)
        editCord.src = nullptr;        
      else
      {
        editCord.src = objVec.at(epIdx).p;
        editCord.src_port = portNum;      
      }
    }
  }
}

void CordEditor::enter(void)
{
  enc0.setValue(epIdx);
  enc0.setLimits(0,objVec.size() -1);
  
  enc2.setLimits(0,1);
  enc2.setValue(1); 
  
  ShowSelection(0);
}

void CordEditor::exit(void)
{
  greyOut(nothing);
}

void CordEditor::greyOut(srctype s)
{
  for (auto obj : objVec)
  {
    bool grey = false;
    if (s == noSrc && 0 == obj.p->objP->outputs) grey = true;
    if (s == noDst && 0 == obj.p->objP->inputs)  grey = true;
    display.DrawAudioObject(*obj.p->objP,obj.p->x,obj.p->y,grey);
  }
}
