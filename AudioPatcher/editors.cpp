
#include "editors.h"
extern AudioObjStatic_t objList[];
extern DeleteEditor deleteEditor;
extern ObjEditor objEditor;
extern void setMuteStatus(bool mute);

static PatchcordInstance_t blankPatch;
/*
 ASCII titles from 
 https://patorjk.com/software/taag/#p=display&c=c%2B%2B&f=Colossal&t=Base%0A
 */
//======================================================================
//
//  888888b.                              
//  888  "88b                             
//  888  .88P                             
//  8888888K.   8888b.  .d8888b   .d88b.  
//  888  "Y88b     "88b 88K      d8P  Y8b 
//  888    888 .d888888 "Y8888b. 88888888 
//  888   d88P 888  888      X88 Y8b.     
//  8888888P"  "Y888888  88888P'  "Y8888  
//
//======================================================================
FLASHMEM AudioObjInstance* BaseEditor::highlightObj(AudioObjInstance* it, uint16_t colour)
{
  if (nullptr != it)
  {
    if (display.canvasMakeVisible(*it, CANVAS_STEP, CANVAS_STEP))
      drawAll();
    display.HighlightAudioObject(it->x, it->y, colour);
  }

  return it;
}

FLASHMEM AudioObjInstance* BaseEditor::highlightObjnum(int n, uint16_t colour)
{
  AudioObjInstance* it = nullptr;

  if (!objVec.empty() && n >= 0 && n < (int) objVec.size())
  {
    it = objVec.at(n).p;
    highlightObj(it, colour);
  }

  return it;
}

// Draw all objects, keeping the bottom line intact
FLASHMEM void BaseEditor::drawAll(bool drawCords /* = true */)
{
  display.SaveStatus();

  for (auto obj : objVec)
    display.DrawAudioObject(*obj.p);

  if (drawCords)
    for (auto cord : cordVec)
      display.DrawPatchcord(cord);

  display.RestoreStatus();
}


// Search object list for (the first) one that overlaps
// the given screen co-ordinates.
// \return index, or -1 if point isn't in an object
FLASHMEM int BaseEditor::PointToObject(int x, int y)
{
  int result = -1;
  AudioPatcherDisplay::side side = AudioPatcherDisplay::side::out;

  for (unsigned int i = 0; i < objVec.size(); i++)
    if (AudioPatcherDisplay::side::out != (side = display.PointIsInObj(*objVec.at(i).p, x, y)))
    {
      result = (int) i;
      break;
    }

  where = side; // store which side of the object we're in, for patchcord editor
  return result;
}


// Search cord list for the closest one to
// the given screen co-ordinates.
// \return index, or -1 if point isn't in an object
FLASHMEM int BaseEditor::PointToCord(int x, int y)
{
  int result = -1;
  int16_t minDist = 32767;

  for (unsigned int i = 0; i < cordVec.size(); i++)
  {
    int16_t tmp = display.PointDistanceToPatchcord(*cordVec.at(i), x, y);
    if (tmp >= 0 && tmp < CORD_SELECT_MIN && tmp < minDist)
    {
      result = (int) i;
      minDist = tmp;
    }
  }
  return result;
}


// Change object selection based on encoder value,
// or force selection to a specific value
FLASHMEM void BaseEditor::SelectByEncoder(LimitedEncoder& enc0, int32_t force)
{
  if (force != NO_FORCE)
    enc0.setValue(force);

  if (force != NO_FORCE || enc0.available())
  {
    highlightObjnum(epIdx, ILI9341_BLACK);
    epIdx = enc0.getValue();
    highlightObjnum(epIdx, ILI9341_WHITE);
  }
}


// Change object selection based on screen touch
FLASHMEM int BaseEditor::SelectByTouch(LimitedEncoder& enc0, bool onlySetEncoder)
{
  int idx = -1;
  if (!touch.isTouched() && touch.isLifted())
  {
    TS_Point lastPoint = touch.getLastPoint();
    idx = PointToObject(lastPoint.x, lastPoint.y);
    if (idx >= 0)
    {
      if (onlySetEncoder)
        enc0.setValue(idx);
      else
        SelectByEncoder(enc0, idx);
    }
  }

  return idx;
}


// Change patchcord selection based on screen touch
FLASHMEM int BaseEditor::SelectCordByTouch(LimitedEncoder& enc0, bool onlySetEncoder)
{
  int idx = -1;
  if (!touch.isTouched() && touch.isLifted())
  {
    TS_Point lastPoint = touch.getLastPoint();
    idx = PointToCord(lastPoint.x, lastPoint.y);
    if (idx >= 0)
    {
      if (onlySetEncoder)
        enc0.setValue(idx);
      else
        SelectByEncoder(enc0, idx);
    }
  }

  return idx;
}


//======================================================================
//
//   .d88888b.  888       d8b                   888             
//  d88P" "Y88b 888       Y8P                   888             
//  888     888 888                             888             
//  888     888 88888b.  8888  .d88b.   .d8888b 888888 .d8888b  
//  888     888 888 "88b "888 d8P  Y8b d88P"    888    88K      
//  888     888 888  888  888 88888888 888      888    "Y8888b. 
//  Y88b. .d88P 888 d88P  888 Y8b.     Y88b.    Y88b.       X88 
//   "Y88888P"  88888P"   888  "Y8888   "Y8888P  "Y888  88888P' 
//                        888                                   
//                       d88P                                   
//                     888P"                                    
//
//======================================================================
FLASHMEM void ObjEditor::ShowSelection(int v)
{
  //Serial.printf("%d : %s\n",v,objList[v].name);
  display.ShowSelection(objList[v].name, objList[v].category);
}


FLASHMEM void ObjEditor::create(int id, int x, int y)
{
  objVec.push_back({new AudioObjInstance(objList[id], x, y)});
}


FLASHMEM void ObjEditor::edit(void)
{
  // Scroll through available object types
  if (enc2.available() || !initialised)
  {
    int v = enc2.getValue();
    lastType = v;
    ShowSelection(v);
  }

  // Place an instance of the currently-selected object type
  if (enc2.wasClicked())
  {
    int16_t x, y, cx, cy;

    display.GetCursor(x, y);
    display.canvasGetCurrent(cx, cy);
    create(enc2.getValue(), cx + x - 24, cy + y - 24);

    AudioObjInstance* ao = objVec.back().p;
    display.CursorClear();
    display.DrawAudioObject(*ao);
    display.CursorRestore();
    std::stable_sort(objVec.begin(), objVec.end());
  }

  // Move the cross-hair cursor
  if (enc0.available(CURSOR_STEP) || enc1.available(CURSOR_STEP) || !initialised)
  {
    bool redraw = false;

    int16_t x = enc0.getValue();
    int16_t y = enc1.getValue();
    if (x < 0)
    {
      display.canvasMoveBy(-CANVAS_STEP, 0); // clears cursor and screen area
      redraw = true;
      x += CURSOR_STEP; // assume just one step over
      enc0.setValue(x);
    }
    if (x > xmax)
    {
      display.canvasMoveBy(CANVAS_STEP, 0);
      redraw = true;
      x -= CURSOR_STEP; // assume just one step over
      enc0.setValue(x);
    }

    if (y < 0)
    {
      display.canvasMoveBy(0, -CANVAS_STEP); // clears cursor and screen area
      redraw = true;
      y += CURSOR_STEP; // assume just one step over
      enc1.setValue(y);
    }
    if (y > ymax)
    {
      display.canvasMoveBy(0, CANVAS_STEP);
      redraw = true;
      y -= CURSOR_STEP; // assume just one step over
      enc1.setValue(y);
    }

    if (redraw)
      drawAll();
    display.CursorTo(x, y);
  }

  if (touch.isTouched())
  {
    //*
    TS_Point p = touch.getPoint();

    enc0.setValue(p.x);
    enc1.setValue(p.y);
    display.CursorTo(p.x, p.y);
    //*/
  }

  if (!initialised)
    initialised = true;
}


//----------------------------------------------------------------------
FLASHMEM void ObjEditor::enter(void)
{
  display.canvasGetLimits(xmax, ymax);
  ymax -= 20; // allow for status line (magic number...)
  enc0.setLimits(-1, xmax + 1);
  enc1.setLimits(-1, ymax + 1);
  enc2.setLimits(1, COUNT_OF_objList);
  enc2.setValue(lastType);
  ShowSelection(enc2.getValue());
}



//======================================================================
//
//  8888888b.          888            888                                   888          
//  888   Y88b         888            888                                   888          
//  888    888         888            888                                   888          
//  888   d88P 8888b.  888888 .d8888b 88888b.   .d8888b .d88b.  888d888 .d88888 .d8888b  
//  8888888P"     "88b 888   d88P"    888 "88b d88P"   d88""88b 888P"  d88" 888 88K      
//  888       .d888888 888   888      888  888 888     888  888 888    888  888 "Y8888b. 
//  888       888  888 Y88b. Y88b.    888  888 Y88b.   Y88..88P 888    Y88b 888      X88 
//  888       "Y888888  "Y888 "Y8888P 888  888  "Y8888P "Y88P"  888     "Y88888  88888P' 
//  
//======================================================================
FLASHMEM void CordEditor::ShowSelection(int io)
{
  char buf[5];

  sprintf(buf, "%s", io ? "dst" : "src");
  display.ShowSelection(buf, AudioCategory_patchcord);
  greyOut(io ? noDst : noSrc);
  drawAll(false);
}

//----------------------------------------------------------------------
FLASHMEM void CordEditor::highlightPort(AudioObjInstance* aoi, int io, int n, bool on)
{
  uint16_t colour = on ? PATCHCORD_COLOUR : CONNECTION_COLOUR;
  if (nullptr != aoi)
  {
    display.DrawConnection(*aoi->objP, aoi->x, aoi->y, n, 0 == io, colour);
    // Serial.printf("highlight %s.%d: %s\n",io?"input":"output",n,on?"on":"off");
  }
}


FLASHMEM void CordEditor::highlightPort(int epIdx, int io, int n, bool on)
{
  if (epIdx >=0 && epIdx < (int) objVec.size())
    highlightPort(objVec.at(epIdx).p, io, n, on);
}

//----------------------------------------------------------------------
FLASHMEM int CordEditor::findGoodObj(int epIdx, int ec1, int io)
{
  AudioObjInstance* aoi;
  int dir = ec1 > epIdx
            ? 1
            : (ec1 < epIdx ? -1 : 0);

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

FLASHMEM int CordEditor::findGoodPort(int epIdx, int portNum, int io)
{
  int port = -1; // assume we'll fail
  AudioObjInstance* aoi = objVec.at(epIdx).p;

  do
  {
    if (1 == io)  // we're setting destination...
    {
      if (0 == aoi->inputAvailFlags) // nothing left, give up
        break;

      if (aoi->inputAvailFlags & (1 << portNum)) // already OK
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


FLASHMEM int CordEditor::findBestSettings(settings& ns, Prioritise pri)
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
          newObj = findGoodObj(epIdx, ns.objNum, ns.srcdst);
          if (newObj >= 0) break; // this is an object which we can use

          if (epIdx == ns.objNum)
          {
            newObj = findGoodObj(epIdx, epIdx + 1, ns.srcdst);
            if (newObj >= 0) break; // this is an object which we can use
          }

          newObj = findGoodObj(epIdx, 2 * epIdx - ns.objNum, ns.srcdst); // search the other way
        } while (0);

        if (newObj >= 0) // this is an object which we can use...
        {
          ns.objNum = newObj; // ...use it ...
          result = findBestSettings(ns, Prioritise::port); // ...and find the best port, too
        }
        else
        {
          if (depth < 2)
          {
            ns.srcdst = 1 - ns.srcdst;
            result = findBestSettings(ns, Prioritise::object);
            if (result < 0) // no luck
              ns.srcdst = 1 - ns.srcdst;
          }
          else
            result = -1; // can't do anything
        }
      }
      break;

    case Prioritise::port: // port changed, prioritise that
      {
        int newPort = findGoodPort(ns.objNum, ns.portNum, ns.srcdst);
        if (newPort >= 0)
          ns.portNum = newPort;
        else
          result = -1;
      }
      break;

    case Prioritise::srcdst: // src/dst changed, prioritise that
      {
        int newObj = findGoodObj(ns.objNum, ns.objNum, ns.srcdst); // current object OK?
        if (newObj < 0) // nope
        {
          ns.objNum = epIdx + 1;
          result = findBestSettings(ns, Prioritise::object); // try to find a workable object
        }
        else
        {
          ns.objNum = newObj; // ...use it ...
          result = findBestSettings(ns, Prioritise::port); // ...and find the best port, too
        }
      }
      break;
  }

  depth--;
  return result;
}


FLASHMEM void CordEditor::edit(void)
{
  bool changedIdx = false;
  int io = 1 - enc2.getValue(); // i = 1 = input = dst, o = 0 = output = src
  CordEditor::settings newSettings = {epIdx, portNum, io};
  bool redrawSelected = false;

  //-----------------------------------------------
  if (touch.isTouched())
  {
    TS_Point p = touch.getSmoothedPoint();
    settings newTouch = {-1};
    int objNum = PointToObject(p.x,p.y);

    if (objNum > 0) // actually in an object
    {
      AudioObjInstance* aoi = objVec.at(objNum).p;

      if (objNum != touchedObj.objNum) // not the same as before
      {
        SelectByEncoder(enc0,objNum);
      }
      newTouch.objNum = objNum;
      newTouch.srcdst = getSide() == -1
                            ?1  // left : destination
                            :0; // right: source
      newTouch.portNum = display.PointToPort(*objVec.at(objNum).p,p.x,p.y);

      if (1 == newTouch.srcdst // destination
        && newTouch.portNum >= 0
        && 0 == (aoi->inputAvailFlags & (1<<newTouch.portNum)))
        newTouch.portNum = -1; // is not available!
      
      // have we changed port? If so, redraw
      if (newTouch.portNum >=0 && 
          (newTouch.portNum != touchedObj.portNum
        || newTouch.srcdst != touchedObj.srcdst))
      {
        highlightPort(touchedObj.objNum,touchedObj.srcdst,touchedObj.portNum,false);
        touchedObj = newTouch;
        highlightPort(touchedObj.objNum,touchedObj.srcdst,touchedObj.portNum,true);
      }
      else 
        touchedObj = newTouch;
    }
    else // touched, but not in an object
    {
      if (touchedObj.objNum >= 0)
      {
        //highlightPort(objVec.at(touchedObj.objNum).p,touchedObj.srcdst,touchedObj.portNum,false);
        highlightObjnum(touchedObj.objNum,ILI9341_BLACK);
      }
      touchedObj.objNum = -1;
    }  
    greyOut(touchNothing); 
  }
  else // no touch at the moment, but...
  {
    char buf[50], *p=buf;

    if (touchedObj.objNum  >= 0  // we selected something
     && touchedObj.portNum >= 0) // which we can use
    {
      if (1 == touchedObj.srcdst) // destination: store that end of the editCord
      {
        editCord.dst      = objVec.at(touchedObj.objNum).p;
        editCord.dst_port = touchedObj.portNum;
      }
      else // source: store
      {
        editCord.src      = objVec.at(touchedObj.objNum).p;
        editCord.src_port = touchedObj.portNum;
      }

      // update bottom text line for confirmation
      if (nullptr != editCord.src)
        p += sprintf(p,"%4.4s.%d",editCord.src->objP->label,editCord.src_port);
      else
        p += sprintf(p,"~~~~~~");
      p += sprintf(p," -> ");
      if (nullptr != editCord.dst)
        p += sprintf(p,"%4.4s.%d",editCord.dst->objP->label,editCord.dst_port);
      else
        p += sprintf(p,"~~~~~~");
      
      display.ShowBottomText(buf);

      // flag ready for encoder press to cxreate patchcord
      setByTouch = nullptr != editCord.src && nullptr != editCord.dst;

      // show status on encoder LED
      if (setByTouch)
        enc1.setLED(0x2F00'2000);
      else
        enc1.setLED(0x2F00'0020);
      
      touchedObj.objNum = -1; // remains until another touch
    }
  }

  //-----------------------------------------------
  // select an audio object
  if (enc0.available())
  {
    int tmp = newSettings.objNum = enc0.getValue();
    findBestSettings(newSettings, Prioritise::object);
    if ((tmp > epIdx) == (newSettings.objNum > epIdx))
      redrawSelected = true;
    else
    {
      enc0.setValue(epIdx);
      newSettings = {epIdx, portNum, io};
    }
    setByTouch = false;
  }

  // select a connection port
  if (changedIdx || enc1.available())
  {
    int tmp = newSettings.portNum = enc1.getValue();
    findBestSettings(newSettings, Prioritise::port);
    if ((tmp > portNum) == (newSettings.portNum > portNum))
      redrawSelected = true;
    else
    {
      enc1.setValue(portNum);
      newSettings = {epIdx, portNum, io};
    }
    setByTouch = false;
    enc1.setLED(0x2F00'0000);
  }

  if (enc2.available()) // src / dst switch
  {
    newSettings.srcdst = 1 - enc2.getValue();
    findBestSettings(newSettings, Prioritise::srcdst);
    redrawSelected = true;
    setByTouch = false;
    enc1.setLED(0x2F00'0000);
  }

  //-----------------------------------------------
  AudioObjInstance* aoi = objVec.at(epIdx).p;

  if (portNum != newSettings.portNum || epIdx != newSettings.objNum)
  {
    highlightPort(aoi, io, portNum, false);
    portNum = newSettings.portNum + 1; // force port highlight later
  }

  if (epIdx != newSettings.objNum)
    SelectByEncoder(enc0, newSettings.objNum);

  if (io != newSettings.srcdst)
  {
    io = newSettings.srcdst;
    ShowSelection(io);
    enc2.setValue(1 - io);
    portNum = newSettings.portNum + 1; // force port highlight later
  }

  if (portNum != newSettings.portNum)
  {
    portNum = newSettings.portNum;
    highlightPort(aoi, io, portNum, true);
    enc1.setValue(portNum); // in case we skipped some
  }

  if (redrawSelected)
  {
    if (nullptr != editCord.src)
      highlightPort(editCord.src, 0, editCord.src_port, true);
    if (nullptr != editCord.dst)
      highlightPort(editCord.dst, 1, editCord.dst_port, true);
  }

  //-----------------------------------------------
  // select a source or destination
  if (enc1.wasClicked())
  {
    AudioObjInstance* aoi = objVec.at(epIdx).p;

    if (!setByTouch)
    {
      if (1 == io) // set dst / input
      {
        highlightPort(editCord.dst, 1, editCord.dst_port, false);
        if (editCord.dst == aoi)
        {
          //Serial.println("dst cleared");
          editCord.dst = nullptr;
        }
        else
        {
          //Serial.println("dst set");
          highlightObj(editCord.dst, ILI9341_BLACK);
          editCord.dst = aoi;
          editCord.dst_port = portNum;
          highlightPort(editCord.dst, 1, editCord.dst_port, true);
        }
      }
      else // set src / output
      {
        highlightPort(editCord.src, 0, editCord.src_port, false);
        if (editCord.src == aoi)
        {
          //Serial.println("src cleared");
          editCord.src = nullptr;
        }
        else
        {
          //Serial.println("src set");
          highlightObj(editCord.src, ILI9341_BLACK);
          editCord.src = aoi;
          editCord.src_port = portNum;
          highlightPort(editCord.src, 0, editCord.src_port, true);
        }
      }
    }

    // both set - create a new patchcord
    if (nullptr != editCord.src && nullptr != editCord.dst) 
    {
      cordVec.push_back(new PatchcordInstance_t{editCord.src, editCord.src_port, editCord.dst, editCord.dst_port}); // create
      display.DrawPatchcord(cordVec.back()); // draw
      highlightPort(editCord.src, 0, editCord.src_port, false); // un-highlight...
      highlightPort(editCord.dst, 1, editCord.dst_port, false); // ...the ports...
      if (aoi == editCord.src)
        highlightObj(editCord.dst, ILI9341_BLACK); // ...and the other...
      if (aoi == editCord.dst)
        highlightObj(editCord.src, ILI9341_BLACK); // ...audio object!

      editCord = blankPatch;
      setByTouch = false;
      enc1.setLED(0x2F00'0000);

      int ec1;
      do
      {
        ec1 = findGoodObj(epIdx, epIdx, io); // can we stay on this object?
        if (ec1 >= 0) // yes
          break;

        ec1 = findGoodObj(epIdx, epIdx + 1, io); // can we find a later one?
        if (ec1 >= 0) // yes
          break;

        ec1 = findGoodObj(epIdx, epIdx - 1, io); // can we find an earlier one?
        if (ec1 >= 0) // yes
          break;

        io = 1 - io; // try swapping src <-> dst

        ec1 = findGoodObj(epIdx, epIdx, io); // can we stay on this object?
        if (ec1 >= 0) // yes
          break;

        ec1 = findGoodObj(epIdx, epIdx + 1, io); // can we find a later one?
        if (ec1 >= 0) // yes
          break;

        ec1 = findGoodObj(epIdx, epIdx - 1, io); // can we find an earlier one?
        if (ec1 >= 0) // yes
          break;

      } while (0);

      if (ec1 >= 0 && (ec1 != epIdx || io != enc2.getValue()))
      {
        if (io != enc2.getValue()) // had to swap src <-> dst
        {
          ShowSelection(io);
          enc2.setValue(1 - io);
        }

        if (ec1 != epIdx)
          SelectByEncoder(enc0, ec1);
      }
    }
  }
}


FLASHMEM void CordEditor::enter(void)
{
  AudioObjInstance* aoi;

  enc0.setLimits(0, objVec.size() - 1);
  enc0.setValue(epIdx);

  enc2.setLimits(0, 1);
  enc2.setValue(1);

  ShowSelection(0);
  aoi = highlightObjnum(epIdx, ILI9341_WHITE);
  editCord = blankPatch;

  portNum = 0;
  highlightPort(aoi, 0, portNum, true);
}

FLASHMEM void CordEditor::exit(void)
{
  highlightObjnum(epIdx, ILI9341_BLACK);
  greyOut(nothing);
  drawAll(false);
  enc1.setLED(0x2F00'0000);
}

FLASHMEM void CordEditor::greyOut(srctype s)
{
  for (auto obj : objVec)
  {
    bool grey = false;
    if (s == noSrc && 0 == obj.p->objP->outputs) grey = true;
    if (s == noDst && (0 == obj.p->objP->inputs || 0 == obj.p->inputAvailFlags))  grey = true;
    obj.p->drawInGrey = grey;
  }
}


//======================================================================
//
//  8888888b.                                                 888                             
//  888   Y88b                                                888                             
//  888    888                                                888                             
//  888   d88P 8888b.  888d888 8888b.  88888b.d88b.   .d88b.  888888 .d88b.  888d888 .d8888b  
//  8888888P"     "88b 888P"      "88b 888 "888 "88b d8P  Y8b 888   d8P  Y8b 888P"   88K      
//  888       .d888888 888    .d888888 888  888  888 88888888 888   88888888 888     "Y8888b. 
//  888       888  888 888    888  888 888  888  888 Y8b.     Y88b. Y8b.     888          X88 
//  888       "Y888888 888    "Y888888 888  888  888  "Y8888   "Y888 "Y8888  888      88888P' 
//  
//======================================================================
FLASHMEM void ParamEditor::enter(void)
{
  enc0Stash = new LimitedEncoderStash(enc0);
  enc1Stash = new LimitedEncoderStash(enc1);
  enc2Stash = new LimitedEncoderStash(enc2);

  display.ShowBottomText("", ILI9341_BLACK);
  enc0.setLimits(0, objVec.size() - 1);
  epIdx = enc0.getValue();
  highlightObjnum(epIdx, ILI9341_WHITE);
  inTarget = false;

  enc1.setLimits(-1, 1);
  enc2.setLimits(-1, 1);

  enc1.setValue(0);
  enc2.setValue(0);
}


FLASHMEM void ParamEditor::exit(void)
{
  highlightObjnum(epIdx, ILI9341_BLACK);
  delete enc0Stash; enc0Stash = nullptr;
  delete enc1Stash; enc1Stash = nullptr;
  delete enc2Stash; enc2Stash = nullptr;

  // might have moved - sort into correct order for saving
  std::stable_sort(objVec.begin(), objVec.end());
}


FLASHMEM void ParamEditor::edit(void)
{
  AudioObjInstance* aoi = objVec.at(epIdx).p;

  if (!inTarget) // we're active, claim the UI
  {
    //-----------------------------------------------
    // select an audio object
    SelectByEncoder(enc0);
    SelectByTouch(enc0);

    // move an audio object
    if (enc1.available() || enc2.available())
    {
      AudioObjInstance* aoi = objVec.at(epIdx).p;
      int dx = enc1.getValue();
      int dy = enc2.getValue();

      enc1.setValue(0);
      enc2.setValue(0);

      aoi->x += dx * CURSOR_STEP;
      aoi->y += dy * CURSOR_STEP;
      if (!display.canvasMakeVisible(*aoi, CANVAS_STEP, CANVAS_STEP)) // didn't blank...
        display.canvasMoveBy(0, 0); //...but want to force a re-draw - do a dummy move

      drawAll();
      highlightObjnum(epIdx, ILI9341_WHITE);
    }

    if (enc0.wasClicked())
    {
      inTarget = true;
      enc0Stash2 = new LimitedEncoderStash(enc0);
      aoi->objP->editFn(aoi, AudioEditMode::enter, nullptr);
      lockModeEncoder();
    }
    
  }
  else // target object has claimed the UI
  {
    if (0 == aoi->objP->editFn(aoi, AudioEditMode::edit, nullptr))
    {
      aoi->objP->editFn(aoi, AudioEditMode::exit, nullptr); // tell editor to tidy up
      inTarget = false; // target has yielded UI control
      if (nullptr != enc0Stash)
      {
        delete enc0Stash2;
        enc0Stash2 = nullptr;
      }
      unlockModeEncoder();
    }
  }
}

//======================================================================
//
//  888b     d888 8888888 8888888b. 8888888 
//  8888b   d8888   888   888  "Y88b  888   
//  88888b.d88888   888   888    888  888   
//  888Y88888P888   888   888    888  888   
//  888 Y888P 888   888   888    888  888   
//  888  Y8P  888   888   888    888  888   
//  888   "   888   888   888  .d88P  888   
//  888       888 8888888 8888888P" 8888888 
// 
//======================================================================
FLASHMEM void MIDIEditor::enter(void)
{
  display.ShowBottomText("", ILI9341_BLACK);
  enc0.setLimits(0, objVec.size() - 1);
  epIdx = enc0.getValue();
  highlightObjnum(epIdx, ILI9341_WHITE);
  inTarget = false;
}


FLASHMEM void MIDIEditor::exit(void)
{
  highlightObjnum(epIdx, ILI9341_BLACK);
}


FLASHMEM void MIDIEditor::edit(void)
{
  AudioObjInstance* aoi = objVec.at(epIdx).p;

  if (!inTarget) // we're active, claim the UI
  {
    //-----------------------------------------------
    // select an audio object
    SelectByEncoder(enc0);
    SelectByTouch(enc0);

    if (enc0.wasClicked())
    {
      // see if object provides any MIDI parameters
      if (aoi->objP->editFn(aoi, AudioEditMode::MIDIenter, nullptr))
      {
        inTarget = true;
        enc0Stash = new LimitedEncoderStash(enc0);
        lockModeEncoder();
      }
    }
  }
  else // target object has claimed the UI
  {
    if (0 == aoi->objP->editFn(aoi, AudioEditMode::MIDIedit, nullptr))
    {
      aoi->objP->editFn(aoi, AudioEditMode::MIDIexit, nullptr); // tell editor to tidy up
      inTarget = false; // target has yielded UI control
      if (nullptr != enc0Stash)
      {
        delete enc0Stash;
        enc0Stash = nullptr;
      }
      unlockModeEncoder();
    }
  }
}


/*======================================================================
//                                                                                                                 
//  8888888b.           888          888            
//  888  "Y88b          888          888            
//  888    888          888          888            
//  888    888  .d88b.  888  .d88b.  888888 .d88b.  
//  888    888 d8P  Y8b 888 d8P  Y8b 888   d8P  Y8b 
//  888    888 88888888 888 88888888 888   88888888 
//  888  .d88P Y8b.     888 Y8b.     Y88b. Y8b.     
//  8888888P"   "Y8888  888  "Y8888   "Y888 "Y8888
// 
//====================================================================*/
FLASHMEM void DeleteEditor::kill(int idx)
{
  AudioObjInstance* aoi = objVec.at(idx).p;

  if (!aoi->noDelete) // I/O and control are protected
  {
    // delete associated patchcords: go backwards
    // so we don't change the index of cords we
    // haven't checked yet
    for (int i = cordVec.size() - 1; i >= 0; i--)
    {
      auto cord = cordVec.at(i);
      if (cord->src == aoi || cord->dst == aoi)
      {
        display.DrawPatchcord(cord, ILI9341_BLACK);
        delete cord;
        cordVec.erase(std::next(cordVec.begin(), i));
      }
    }

    // now we can delete the audio object
    display.EraseAudioObject(*aoi->objP, aoi->x, aoi->y); // from the display...
    objVec.erase(std::next(objVec.begin(), idx));
    delete aoi; // ...and from memory
  }
}

FLASHMEM void DeleteEditor::enter(void)
{
  delType = delObj;

  enc0.setValue(epIdx);
  enc0.setLimits(0, objVec.size() - 1);
  highlight(-1, epIdx);

  enc2.setLimits(0, 1);
  enc2.setValue(delType);
  ShowSelection(enc2.getValue());
}

FLASHMEM void DeleteEditor::exit(void)
{
  highlight(epIdx, -1);
}

FLASHMEM void DeleteEditor::edit(void)
{
  // select an audio object or patchcord
  if (enc0.available() ||
      (delObj == delType
       ? SelectByTouch(enc0, true)
       : SelectCordByTouch(enc0, true)
      ) >= 0)
  {
    int ec1 = enc0.getValue();
    //Serial.printf("Delete highlight %d -> %d\n", epIdx, ec1);
    highlight(epIdx, ec1);
    epIdx = ec1;
  }


  if (enc2.available())
  {
    highlight(epIdx, -1);
    delType = (delType_e) enc2.getValue();
    ShowSelection(delType);
    enc0.setLimits(0, delType == delObj
                   ? (objVec.size() - 1)
                   : (cordVec.size() - 1));
    epIdx = enc0.getValue();
    highlight(-1, epIdx);
  }

  if (enc0.wasClicked())
  {
    switch (delType) // time to delete a thing!
    {
      case delObj:
        {
          kill(epIdx);
        }
        break;

      case delCord:
        {
          PatchcordInstance_t* ppc = cordVec.at(epIdx);
          display.DrawPatchcord(ppc, ILI9341_BLACK);
          delete ppc;
          cordVec.erase(std::next(cordVec.begin(), epIdx));
          enc0.setLimits(0, cordVec.size() - 1);
        }
        break;
    }
    Serial.printf("We have %d objects and %d patchcords\n", objVec.size(), cordVec.size());

    // selection has changed, update display
    epIdx = enc0.getValue();
    highlight(-1, epIdx);
  }
  
}


FLASHMEM void DeleteEditor::highlight(int remove, int add)
{
  switch (delType)
  {
    case delObj:
      if (remove >= 0)
      {
        AudioObjInstance* aoi =
          highlightObjnum(remove, ILI9341_BLACK); // take highlight off old object
        for (auto cord : cordVec) // highlight attached patchcords, too
          if (cord->src == aoi || cord->dst == aoi)
            display.DrawPatchcord(cord);
      }
      if (add >= 0)
      {
        AudioObjInstance* aoi = objVec.at(add).p;
        uint16_t colour = aoi->noDelete ? ILI9341_RED : ILI9341_WHITE;

        highlightObjnum(add, colour); // put it on the new one
        if (!aoi->noDelete) // only if deletable
        {
          for (auto cord : cordVec) // highlight attached patchcords, too
            if (cord->src == aoi || cord->dst == aoi)
              display.DrawPatchcord(cord, PATCHCORD_HIGHLIGHT);
        }
      }
      break;

    case delCord:
      if (remove >= 0)
        display.DrawPatchcord(cordVec.at(remove)); // take highlight off old object
      if (add >= 0)
      {
        if (display.canvasMakeVisible(*cordVec.at(add), CANVAS_STEP, CANVAS_STEP))
          drawAll();
        display.DrawPatchcord(cordVec.at(add), PATCHCORD_HIGHLIGHT); // put it on the new one
      }
      break;
  }
}

FLASHMEM void DeleteEditor::ShowSelection(int op)
{
  char buf[20];

  sprintf(buf, "%s", op ? "patchcords" : "objects");
  display.ShowBottomText(buf, display.getModeColour());
}


//======================================================================
//
//  8888888888 d8b 888                   
//  888        Y8P 888                   
//  888            888                   
//  8888888    888 888  .d88b.  .d8888b  
//  888        888 888 d8P  Y8b 88K      
//  888        888 888 88888888 "Y8888b. 
//  888        888 888 Y8b.          X88 
//  888        888 888  "Y8888   88888P' 
//                    
//======================================================================
FLASHMEM void makeFFP(char* buf, const char* base, const char* path, const char* leaf, const char* ext)
{
  char* p = buf; // assumed that caller 
  if (nullptr != base)
    p += sprintf(p, "%s/", base);  
  if (nullptr != path && 0 != *path)
    p += sprintf(p, "%s/", path);
  p += sprintf(p, "%s", leaf);
  if (nullptr != ext && 0 != *ext)
  {
    if ('.' != *ext)
      *p++ = '.';
    p += sprintf(p, "%s", ext);
  }
}

static const char* lastFile = "!last.txt";
static const size_t LAST_FILE_LEN = 9;
FLASHMEM void FileBase::setLast(const char* nme)
{
  File saveTo;
  char buf[basePathLen + LAST_FILE_LEN + 1];

  makeFFP(buf, basePath, nullptr, lastFile, nullptr);
  saveTo = SD.open(buf, FILE_WRITE_BEGIN);
  if (saveTo)
  {
    saveTo.truncate();
    makeFFP(buf, basePath, filePath, nme, nullptr);
    saveTo.print(buf); 
    saveTo.print(NAME_EOL);
    saveTo.close();
  }
}


// last file used, or -1
FLASHMEM int FileBase::getLast(char* buf, int maxn)
{
  int result = -1;
  File loadFrom;
  char lastFFP[basePathLen + LAST_FILE_LEN + 1];

  makeFFP(lastFFP, basePath, nullptr, lastFile, nullptr);

  loadFrom = SD.open(lastFFP, FILE_READ);
  if (loadFrom)
  {
    result = loadFrom.readBytesUntil(NAME_EOL, buf, maxn);
    loadFrom.close();
  }

  return result;
}


FLASHMEM void FileEditor::del(const char* nme)
{
  char buffer[200];
  File saveTo;

  makeFFP(buffer, basePath, filePath, nme, ".txt");
  Serial.printf("Delete %s\n", buffer);
  SD.remove(buffer);
}


FLASHMEM void FileEditor::save(const char* nme)
{
  char buffer[200];
  File saveTo;
  int count;

  makeFFP(buffer, basePath, filePath, nme, ".txt");
  Serial.printf("Save to %s\n", buffer);
  SD.remove(buffer);
  saveTo = SD.open(buffer, FILE_WRITE_BEGIN);

  if (saveTo)
  {
    count = 0;
    for (auto obj : objVec)
    {
      snprintf(buffer, 50,
               "#%d:%c %s @ %d,%d%s\n",
               count++,
               obj.p->perVoice ? OBJ_PER_VOICE_CHAR : ' ',
               obj.p->objP->name,
               obj.p->x, obj.p->y,
               obj.p->noDelete ? " *" : ""
              );
      asm("nop");
      Serial.print(buffer); Serial.flush();
      saveTo.write(buffer, strlen(buffer));
    }

    for (auto cord : cordVec)
    {
      const int UNSET = 999'999'999;
      int src = UNSET, dst = UNSET;

      count = 0;
      for (auto obj : objVec)
      {
        if (obj.p == cord->src) src = count;
        if (obj.p == cord->dst) dst = count;
        if (src != UNSET && dst != UNSET)
          break;
        count++;
      }

      Serial.printf("%d.%d -> %d.%d\n", src, cord->src_port, dst, cord->dst_port);
      saveTo.printf("%d.%d -> %d.%d\n", src, cord->src_port, dst, cord->dst_port);
    }

    getSetParams gsp{buffer};
    for (size_t i = 0; i < objVec.size() - 1; i++)
    {
      AudioObjInstance* aoi = objVec.at(i).p;
      gsp.sz = 190;
      if (aoi->objP->editFn(aoi, AudioEditMode::getParams, &gsp)) // see if object has settings, if so...
      {
        saveTo.printf("~%d: %s\n", i, gsp.buffer); // ... save those, too
        Serial.printf("~%d: %s\n", i, gsp.buffer);
      }
    }

    for (size_t i = 0; i < objVec.size() - 1; i++)
    {
      AudioObjInstance* aoi = objVec.at(i).p;
      gsp.sz = 190;
      if (aoi->objP->editFn(aoi, AudioEditMode::getMIDIparams, &gsp)) // see if object has MIDI settings, if so...
      {
        saveTo.printf("@%d: %s\n", i, gsp.buffer); // ... save those, too
        Serial.printf("@%d: %s\n", i, gsp.buffer);
      }
    }

    Serial.print("truncating at "); Serial.flush();
    size_t sz = saveTo.position();
    Serial.print(sz); Serial.flush();
    saveTo.truncate(sz);

    Serial.print(" ... closing\n"); Serial.flush();
    saveTo.close();
    Serial.print("Saved\n"); Serial.flush();

    setLast(nme);
  }
}


FLASHMEM void FileEditor::dump(const char* nme)
{
  char buffer[100];
  File loadFrom;

  makeFFP(buffer, basePath, filePath, nme, ".txt");
  loadFrom = SD.open(buffer, FILE_READ);

  if (loadFrom)
  {
    int got;

    loadFrom.setTimeout(1);
    do
    {
      got = loadFrom.readBytesUntil('\n', buffer, 49);
      if (got > 0)
      {
        if (got == 49)
          Serial.print(buffer);
        else
          Serial.println(buffer);
      }
    } while (got > 0);

    loadFrom.close();
  }
}


FLASHMEM void FileEditor::load(const char* nme)
{
  char buffer[200];
  File loadFrom;

  makeFFP(buffer, basePath, filePath, nme, ".txt");
  Serial.printf("Load from %s\n", buffer);
  loadFrom = SD.open(buffer, FILE_READ);

  if (loadFrom)
  {
    int got;
    int ndCount = 0; // count of nondeletable objects

    loadFrom.setTimeout(1);

    setMuteStatus(true);

    // remove existing patch: deleting the objects will
    // automagically remove the associated patchcords
    for (int i = objVec.size() - 1; i >= 0; i--)
    {
      AudioObjInstance* aoi = objVec.at(i).p;

      if (!aoi->noDelete)
        deleteEditor.kill(i);
    }
    Serial.printf("We have %d objects and %d patchcords remaining\n", objVec.size(), cordVec.size());


    do // load objects
    {
      int n, id, x, y, nd;
      char objname[30], perVoice;

      got = loadFrom.readBytesUntil('\n', buffer, 199);
      if (0 == got)
        break;
      buffer[got] = 0;
      Serial.print(buffer);
      if (5 == sscanf(buffer, "#%d:%c %s @ %d,%d", &n, &perVoice, objname, &x, &y))
      {
        if (objname[0] < '0' || objname[0] > '9') // name is text
          id = objNameToID(objname);
        else
          sscanf(objname, "%d", &id);
        nd = buffer[strlen(buffer) - 1] == '*';
        Serial.printf(" ... %d <%c> %s (%d,%d) %s\n", n, perVoice, objList[id].name, x, y, nd ? "protected" : "");

        if (!nd) // can add this object, it's not a non-destructible one
        {
          objEditor.create(id, x, y);
          AudioObjInstance* aoi = objVec.back().p;
          aoi->perVoice = perVoice == OBJ_PER_VOICE_CHAR;
        }
        else // just restore positions
        { // this won't work if the ordering is different - leave for now
          AudioObjInstance* aoi = objVec.at(ndCount).p;
          aoi->x = x;
          aoi->y = y;
          ndCount++;
        }
      }
      else
        break;
    } while (1);

    // objects all loaded, buffer has first patchcord definition
    // make sure objects are in the order the patchcords
    // and parameter settings
    std::stable_sort(objVec.begin(), objVec.end());

    // now make the connections
    Serial.println();
    do
    {
      int src, srcport, dst, dstport;
      if (4 == sscanf(buffer, "%d.%d -> %d.%d", &src, &srcport, &dst, &dstport))
      {
        cordVec.push_back(new PatchcordInstance_t{objVec.at(src).p, (int8_t) srcport, objVec.at(dst).p, (int8_t) dstport}); // create
      }
      else
        break;

      got = loadFrom.readBytesUntil('\n', buffer, 199);
      if (0 == got)
        break;
      Serial.println(buffer);
    } while (1);

    // reset the canvas and draw the patch
    display.canvasMoveTo(0, 0); // does the CursorClear()
    drawAll();
    display.CursorRestore();

    // now get any parameter settings
    do
    {
      int id, off;
      if (1 == sscanf(buffer, "~%d:%n", &id, &off))
      {
        if (id > 0 && (uint32_t) id < objVec.size())
        {
          AudioObjInstance* aoi = objVec.at(id).p;
          getSetParams gsp{buffer + off, strlen(buffer + off)};
          aoi->objP->editFn(aoi, AudioEditMode::setParams, &gsp);
        }
        got = loadFrom.readBytesUntil('\n', buffer, 199);
        if (0 == got)
          break;
        Serial.println(buffer);
      }
      else
        break;
    } while (1);

    // now get any MIDI parameter settings
    do
    {
      int id, off;
      if (1 == sscanf(buffer, "@%d:%n", &id, &off))
      {
        if (id > 0 && (uint32_t) id < objVec.size())
        {
          AudioObjInstance* aoi = objVec.at(id).p;
          getSetParams gsp{buffer + off, strlen(buffer + off)};
          aoi->objP->editFn(aoi, AudioEditMode::setMIDIparams, &gsp);
        }
        got = loadFrom.readBytesUntil('\n', buffer, 199);
        if (0 == got)
          break;
        Serial.println(buffer);
      }
      else
        break;
    } while (1);

    loadFrom.close();

    delay(5); // allow audio system to settle
    setMuteStatus(false);
  }
}

FLASHMEM int FileBase::loadLast(void)
{
  const size_t BUF_LEN = basePathLen + MAX_FILE_PATH + 1 + MAX_FILE_NAME + 1;
  char buf[BUF_LEN];  // maximum possible (?)
  int result = getLast(buf, BUF_LEN);

  if (result > 0)
  {
    char* nme, *path;

    Serial.printf("Last file was: '%s'\n", buf);
    buf[BUF_LEN - 1] = 0; // ensure string is terminated
    path = strchr(buf + 1, '/'); // start of path, after basePath
    nme = strrchr(buf, '/');  // find file leaf name
    if (nullptr != nme) // there was a path separator
    {
      if (nme == path) // if file is at base...
      {
          nme++; // filename is everything after the separator        
          filePath[0] = 0; // start at base path
      }
      else
      {
        *nme = 0; // replace separator with terminator
        nme++; // point to leaf name
        if (strlen(path) < sizeof filePath)
          strcpy(filePath, path + 1); // copy path, omitting leading /
        else
          filePath[0] = 0; // start at base path
      }      
    }
    else
      nme = buf;

    if (strlen(nme) < sizeof fileName)
    {
      strcpy(fileName, nme);
      load(nme);
    }
    else
      fileName[0] = 0;
  }

  return result;
}


void FileBase::showFileList(const int item, bool showAll)
{
  // we assume the item number is sane...
  if (item < fileListTop || item > fileListTop + MAX_FILE_LINE)
  {
    showAll = true;
    while (item < fileListTop && fileListTop > 0) fileListTop -= MAX_FILE_LINE / 2 + 1;
    while (item > fileListTop + MAX_FILE_LINE && fileListTop < (int) fileList.size() - MAX_FILE_LINE) fileListTop += MAX_FILE_LINE / 2 + 1;
    if (fileListTop + MAX_FILE_LINE > (int) fileList.size() - 1) fileListTop = (int) fileList.size() - MAX_FILE_LINE - 1;
    if (fileListTop < 0) fileListTop = 0;
  }


  if (showAll)
  {
    for (int i = 0; i <= MAX_FILE_LINE && i + fileListTop < (int) fileList.size(); i++)
    {
      fileDisplay.ShowAreaText(fileList.at(i + fileListTop).name.c_str(), FILE_X_OFF, FILE_Y_OFF, i,
                           fileList.at(i + fileListTop).isDir
                              ? DIR_NAME_COLOUR
                              : KEY_CAP_COLOUR,
                           i == (item - fileListTop)
                              ? KEY_ACTIVE_BKGND
                              : EDIT_BKGND);
    }
  }
  else
  {
        // clear old highlight
    fileDisplay.ShowAreaText(fileList.at(fileListCurrent).name.c_str(), FILE_X_OFF, FILE_Y_OFF, fileListCurrent - fileListTop,
                         fileList.at(fileListCurrent).isDir
                         ? DIR_NAME_COLOUR
                         : KEY_CAP_COLOUR,
                         EDIT_BKGND);
    // highlight new filename
    fileDisplay.ShowAreaText(fileList.at(item           ).name.c_str(), FILE_X_OFF, FILE_Y_OFF, item            - fileListTop,
                         fileList.at(item).isDir
                         ? DIR_NAME_COLOUR
                         : KEY_CAP_COLOUR,
                         KEY_ACTIVE_BKGND);
  }

  fileListCurrent = item;
}


void FileBase::showMode(bool zapCurrent /* = true */)
{
  char buffer[5 + MAX_FILE_NAME + 1];
  int theMode = enc0.getValue();
  mode = (mode_e) theMode;

  switch (mode) // saving - show keyboard to create filename
  {
    case mode_e::save:
      fileDisplay.ShowKeyboard(20, 40, "File name", !keyboardVisible);
      keyboardVisible = true;
      break;

    case mode_e::load:
    case mode_e::del:
      {
        int16_t x,y,w,h;
        fileDisplay.GetDefaultKeyboardArea(x,y,w,h);

        if (!keyboardVisible)
          fileDisplay.SaveArea(x, y, w, h);
        fileDisplay.InitArea(x, y, w, h);
        fileDisplay.ShowTitle("File list", 5, 5);

        if (zapCurrent)
        {
          int i;
          bool gotFile = false;

          clearFileList();
          createFileList(filePath, mode);
          enc1.setLimits(0, fileList.size() - 1);
          for (i = 1; i < (int) fileList.size(); i++)
            if (!fileList.at(i).isDir)
            {
              gotFile = true;
              break;
            }
              
          if (!gotFile) // no files, only folders
            i = 0 == fileList.size() ? 0 : 1;
          enc1.setValue(i);
          fileListTop = -MAX_FILE_LINE - 1;
          fileListCurrent = -1;
        }
        showFileList(enc1.getValue(), true);
        keyboardVisible = true;
      }
      break;
  }
  const char* labels[] = {"load", "save", " del"};
  sprintf(buffer, "%s:%s", labels[(int) mode], fileName);
  fileDisplay.ShowBottomText(buffer, fileDisplay.getModeColour());
}

FLASHMEM void FileBase::createFileList(const char* path, mode_e theMode)
{
  File root;
  size_t plen = strlen(path);
  char buf[basePathLen + 1 + plen + 1];

  makeFFP(buf, basePath, nullptr, filePath, nullptr);
  root = SD.open(buf);
  fileList.push_back({"..", true});

  while (true)
  {
    File entry = root.openNextFile();

    if (!entry)
      break;

    {
      String nme = String(entry.name());

      if (!nme.startsWith('!')) // ignore ! at start
      {
        if (entry.isDirectory())
          fileList.push_back({nme, true});
        else
        {
          if (nme.endsWith(fileExtn)) // ignore !last.txt, thing.exe etc.
          {
            nme.replace(fileExtn, "");
            fileList.push_back({nme, false});
          }
        }
      }
    }
  }

  std::stable_sort(fileList.begin(), fileList.end());

  //for (auto s : fileList)
  //  Serial.println(s.c_str());
}

FLASHMEM void FileBase::clearFileList(void)
{
  fileList.clear();
}

FLASHMEM void FileBase::enter(bool saveArea /* = true */)
{
  enc0.setLimits(0, (int) maxMode); // load / save / del
  enc0.setValue(0);
  keyboardVisible = !saveArea;

  //initialise filename on entry
  if (idx < 0) // first time ever
  {
    idx = strlen(fileName);
  }
  showMode();
}

FLASHMEM void FileBase::exit(void)
{
  if (keyboardVisible)
    fileDisplay.RestoreArea();
  clearFileList();
}

FLASHMEM void FileBase::newKey(AudioPatcherDisplay::keyInfo key)
{
  static AudioPatcherDisplay::keyInfo lastKey;

  if (key.ch != lastKey.ch)
  {
    if (lastKey.ch != 0)
      fileDisplay.ShowKey(lastKey, KEY_CAP_COLOUR, EDIT_BKGND, upperCase);
    lastKey = key;
    if (lastKey.ch != 0)
      fileDisplay.ShowKey(lastKey, KEY_CAP_COLOUR, KEY_ACTIVE_BKGND, upperCase);
  }
}


FLASHMEM void FileBase::edit(void)
{
  // Deal with touch
  if (keyboardVisible)
  {
    switch (mode)
    {
      case mode_e::save:
        if (touch.isTouched())
        {
          AudioPatcherDisplay::keyInfo key;
          TS_Point p = touch.getPoint();
          key = fileDisplay.KeyAt(p.x, p.y);
          newKey(key);
        }
        else
        {
          if (touch.isLifted())
          {
            AudioPatcherDisplay::keyInfo key;
            TS_Point p = touch.getLastPoint();
            key = fileDisplay.KeyAt(p.x, p.y);

            if (key.ch > 0)
            {
              Serial.print((char) key.ch);
              if (idx < MAX_FILE_NAME)
              {
                char buf[MAX_FILE_NAME + 5 + 1];

                fileName[idx++] = key.ch;
                fileName[idx] = 0;
                sprintf(buf, "save:%s", fileName);
                fileDisplay.ShowBottomText(buf);
              }
            }
            else
            {
              switch (key.ch)
              {
                case -10: // toggle case
                  upperCase = !upperCase;
                  fileDisplay.RedrawKeyboard(upperCase);
                  break;

                case -11: // delete
                  if (idx > 0)
                  {
                    char buf[MAX_FILE_NAME + 5 + 1];

                    idx--;
                    fileName[idx] = 0;
                    sprintf(buf, "save:%s", fileName);
                    fileDisplay.ShowBottomText(buf);
                  }
                  break;

                case -12: // enter
                  Serial.printf("save %s\n", fileName);
                  break;
              }
            }

            key.ch = 0;  // "no key"
            newKey(key); // negate
          }
        }
        break;
        
      case mode_e::load:
      case mode_e::del:
        if (!touch.isTouched() && touch.isLifted()) // tap on file list
        {
          TS_Point p = touch.getLastPoint();
          AudioPatcherDisplay::keyInfo line = fileDisplay.LineAt(p.x,p.y,FILE_X_OFF,FILE_Y_OFF);
          if (line.ch >= -2)
            enc1.setValue(line.ch + fileListTop, true); // fake encoder jump to new value
        }
        break;
    }
  }

  // switch between load, save, del, dir
  if (enc0.available())
    showMode();

  if (enc0.wasClicked())
  {
    switch (mode)
    {
      case mode_e::save:
        save(fileName);
        Serial.printf("\nCheck load of patch %s:\n", fileName);
        dump(fileName);
        Serial.println("---------------\n");
        break;

      case mode_e::load:
        {
          FileListEntry& entry = fileList.at(enc1.getValue());

          if (entry.isDir)
          {
            bool reRead = false;
            if (entry.name == "..") // go up one level, if possible
            {
              char* lastSlash = strrchr(filePath, '/');

              if (lastSlash != filePath)
              {
                if (nullptr == lastSlash) // no path separator...
                  lastSlash = filePath;   // ... path has become empty
                *lastSlash = 0;
                reRead = true;
              }
            }
            else // entry is sub-directory
            {
              const size_t BUF_LEN = basePathLen + 1 + MAX_FILE_PATH + 1 + MAX_FILE_NAME + 1;
              char buf[BUF_LEN];

              sprintf(buf, "%s/%s", filePath, entry.name.c_str()); // append path element
              if (buf[0] == '/') // old path was empty
                strcpy(filePath, buf + 1); // don't prepend /
              else
                strcpy(filePath, buf);
              reRead = true;
            }

            if (reRead) // changed directory
              showMode(true);
          }
          else
          {
            strcpy(fileName, entry.name.c_str());
            idx = strlen(fileName);
            fileDisplay.RestoreArea();
            keyboardVisible = false;
            load(fileName);
            setLast(fileName);
            Serial.println("---------------\n");
            delay(250);
            showMode(false);
          }
        }
        break;

      case mode_e::del:
        if (!fileList.at(enc1.getValue()).isDir)
        {
          Serial.printf("Delete %s/%s\n", filePath, fileList.at(enc1.getValue()).name.c_str());
          del(fileList.at(enc1.getValue()).name.c_str());
          fileList.erase(fileList.begin() + enc1.getValue());
          enc1.setLimits(0, fileList.size()-1);
          showMode(false);
        }
        break;
    }
    state = 0;
  }

  // scroll through file list, if we're not on the "save" screen
  if (enc1.available() && mode_e::save != mode)
    showFileList(enc1.getValue());
}
