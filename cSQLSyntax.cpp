//---------------------------------------------------------------------------

#include <vcl.h>

#pragma hdrstop


#include "cSQLSyntax.h"
#include "uBuffer.h"
#include "uIter.h"
#include "uParser.h"
#include "uDrawer.h"
#include "uLanguageDefinition.h"
#include "uLanguageDefinitionSQL.h"
#include <windows.h>

using namespace SHEdit;

#pragma package(smart_init)
//---------------------------------------------------------------------------
// ValidCtrCheck is used to assure that the components created do not have
// any pure virtual functions.
//

#ifdef DEBUG
#include <fstream>
std::ofstream myfile;
#endif

static inline void ValidCtrCheck(TSQLEdit *)
{
  new TSQLEdit(NULL);
}
//---------------------------------------------------------------------------
namespace Csqlsyntax
{
  void __fastcall PACKAGE Register()
  {
    TComponentClass classes[1] = {__classid(TSQLEdit)};
    RegisterComponents(L"Medicalc", classes, 0);
  }
}
//---------------------------------------------------------------------------

TSQLEdit * SQLEditFocused; //callback musi jit na statickou metodu...

//---------------------------------------------------------------------------
  __fastcall TSQLEdit::TSQLEdit(TComponent* Owner)
: TCustomControl(Owner)
{
#ifdef DEBUG
  myfile.open("main.txt", ios::out );
#endif
  VBar = new TScrollBar(this);
  VBar->ParentWindow = this->ParentWindow;
  VBar->SetParentComponent(this);
  VBar->Align = alRight;
  VBar->Kind = sbVertical;
  VBar->Width = 15;
  VBar->SmallChange = SCROLL_STEP;
  VBar->LargeChange = 10;
  VBar->PageSize = 10;
  VBar->OnScroll = OnVScroll;
  VBar->Hide();

  HBar = new TScrollBar(this);
  HBar->ParentWindow = this->ParentWindow;
  HBar->SetParentComponent(this);
  HBar->Align = alBottom;
  HBar->Kind = sbHorizontal;
  HBar->OnScroll = OnHScroll;
  HBar->LargeChange = 40;
  HBar->Hide();

  OnResize = OnResizeCallback;

  buffer = new Buffer();
  itrLine = buffer->First();
  itrCursor = buffer->End();
  itrCursorSecond = NULL;

  //this->DoubleBuffered = true;

  this->Color = clWhite;

  scrolldelta = 0;
  scrolldeltafontsize = 0;
  cursorLeftOffset = 0;

  recmsg = false;

  selectionFormat = new Format(NULL, new TColor(clGray));
  cursorsInInvOrder = false;

  clipboard = Clipboard();

  drawer = new Drawer(this->Canvas, this);
  parser = new Parser(this, drawer);
  parser->SetLangDef(new LanguageDefinitionSQL());

  if(!ComponentState.Contains(csDesigning))
  {
    //drawer->Start() ;
    //parser->Start() ;
  }
}

//---------------------------------------------------------------------------
__fastcall TSQLEdit::~TSQLEdit()
{
  UnhookWindowsHookEx(KBHook);
  if(this == SQLEditFocused)
    SQLEditFocused = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::Paint()
{
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::PaintWindow(HDC DC)
{
  RepaintWindow(false);
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::RepaintWindow(bool force)
{
  if(!DEBUG_REPAINT || force)
  {
    NSpan * line = itrLine->line;
    for(int j = GetVisLineCount(), i = 0; i < j; i++)
    {
      parser->ParseFromLine(line, itrLine->linenum+i, 2);
      if(line->nextline && line->nextline->nextline)
        line = line->nextline;
      else
        break;
    }
    parser->Execute();
  }
}

//---------------------------------------------------------------------------
LRESULT CALLBACK SHEdit::ProcessKeyCall(int code, WPARAM wParam, LPARAM lParam)
{
  if(SQLEditFocused)
    return SQLEditFocused->ProcessKey(code, wParam, lParam);
  else return 0;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK TSQLEdit::ProcessKey(int code, WPARAM wParam, LPARAM lParam)
{
  bool trap = false;
  parser->processAll = false;
  if((lParam & 2147483648) == 0)
  {
    switch(wParam)
    {
#ifdef DEBUG
      case VK_F10:
        {
          CheckIntegrity();
          CheckIterIntegrity(itrCursor);
        }
        return 1;
        break;
#endif
      case VK_LEFT:
        itrCursor->RevChar();
        AdjustLine(true);
        cursorLeftOffset = itrCursor->GetLeftOffset();
        trap = true;
        break;
      case VK_RIGHT:
        itrCursor->GoChar();
        AdjustLine(true);
        cursorLeftOffset = itrCursor->GetLeftOffset();
        trap = true;
        break;
      case VK_UP:
        itrCursor->RevLine();
        itrCursor->GoByOffset(cursorLeftOffset);
        AdjustLine(true);
        trap = true;
        break;
      case VK_DOWN:
        itrCursor->GoLine();
        itrCursor->GoByOffset(cursorLeftOffset);
        AdjustLine(true);
        trap = true;
        break;
      case VK_TAB:
        //if(tabonce || true) //nessage is apparently called twice  //ehm sometimes
        {
          wchar_t * str = new wchar_t[2];
          str[0] = '\t';
          str[1] = '\0';
          AdjustLine(true);
          Insert(str);
          cursorLeftOffset = itrCursor->GetLeftOffset();
          trap = true;
          
        }
        break;
    }
  }
  if(trap)
  {
    UpdateCursor(true);
    parser->processAll = true;
    return 1;
  }
  else
  {
    parser->processAll = true;
    return 0;
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::Scroll(int by)
{
  if(by == 0)
    return;
  int scrolled = 0;
  if(by < GetVisLineCount())
  {
    while(by != 0)
    {
      if(by > 0)
      {
        if(itrLine->line->prevline)
        {
          itrLine->RevLine();
          scrolled++;
        }
        by--;
      }
      else
      {
        if(itrLine->line->nextline && itrLine->line->nextline->nextline && itrLine->line->nextline->nextline->nextline)
        {
          itrLine->GoLine();
          scrolled--;
        }
        by++;
      }
    }
    VBar->Position = itrLine->linenum;
    VBar->PageSize = GetVisLineCount();
    ProcessChange(0, scrolled, NULL);
    UpdateCursor(true);
  }

}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::WndProc(Messages::TMessage &Message)
{
  bool processed = false;
  NSpan* changed = NULL;
  Iter * itr = NULL;
  int linesMoved = 0;
  int linesMovedFrom = 0;
  switch(Message.Msg)
  {
    case WM_MOUSEWHEEL:
      {
        int virtkey = GetKeyState(VK_CONTROL);
        if (virtkey & 0x8000)
        {
          scrolldeltafontsize += (short)HIWORD(Message.WParam);
          int scrolled = scrolldeltafontsize/120;
          scrolldeltafontsize = scrolldeltafontsize-scrolled*120;
          if(scrolled < 0 && drawer->GetFontsize()+scrolled >= 4)
            drawer->SetFontsize(drawer->GetFontsize()+scrolled);
          if(scrolled > 0 && drawer->GetFontsize()+scrolled <= 30)
            drawer->SetFontsize(drawer->GetFontsize()+scrolled);
          UpdateCursor(false);
          drawer->UpdateLinenumWidth(buffer->GetLineCount());
          RepaintWindow(true);
        }
        else
        {
          scrolldelta += (short)HIWORD(Message.WParam);
          int scrolled = scrolldelta/120;
          scrolldelta = scrolldelta-scrolled*120;
          Scroll(scrolled*SCROLL_STEP);
        }
      }
      return;
    case WM_KEYDOWN:
      if(FOnKeyPress != NULL)
      {
        bool ret = false;
          FOnKeyPress(this, Message.WParam, ret);
          if(ret)
            break;
      }
      switch(Message.WParam)
      {
        case VK_DELETE:
        case VK_BACK:
          if(!itrCursorSecond)
          {
            if(Message.WParam == VK_BACK && !(itrCursor->word->prev->prev || (itrCursor->word->prev && itrCursor->offset > 0)))  //have to stop after the head, not on it
            {
              break;
            }
            if(Message.WParam == VK_DELETE && !(itrCursor->word->next))  //have to stop after the head, not on it
            {
              break;
            }
            itrCursorSecond = itrCursor->Duplicate();
            if(Message.WParam == VK_BACK)
              itrCursor->RevChar();
            else
              itrCursorSecond->GoChar();
          }
          if(itrCursorSecond)
            DeleteSel();
          cursorLeftOffset = itrCursor->GetLeftOffset();
          AdjustLine(true);
          UpdateCursor(true);
          break;
        case VK_F2:
          LoadFile("test.txt");
          return;
        case VK_F5:
          UpdateCursor(false);
          RepaintWindow(true);
          return;
        case VK_F6:
          {
            int justbreaksomewhere = 666;
          }
          return;
#ifdef DEBUG
        case VK_F7:
          {
            parser->dbgLogging = true;
            parser->ParseFromLine(itrCursor->line, itrCursor->linenum, 2);
            parser->Execute();
          }
          return;
        case VK_F8:
          {
            parser->dbgLogging = false;
          }
          return;
#endif
        case VK_F9:
          {
            Iter * itr = itrLine->Duplicate();
            itr->RevLine();
            itr->RevLine();
            itr->MarkupBegin(this->selectionFormat);
            parser->ParseFromLine(itr->line, itr->linenum, 0);
            parser->Execute();
            delete itr;
          }
          return;
        case VK_PRIOR:
        case VK_NEXT:
          cy += drawer->GetLinesize() * GetVisLineCount();
          my += drawer->GetLinesize() * GetVisLineCount();
          dy += drawer->GetLinesize() * GetVisLineCount();
          for(int i = GetVisLineCount(); i > 0; i--)
          {
            if(Message.WParam == VK_PRIOR)
              itrLine->RevLine();
            else
              itrLine->GoLine();
          }
          UpdateCursor(false);
          UpdateVBar();
          RepaintWindow(true);
          break;
        case VK_HOME:
          itrCursor->GoLineStart();
          UpdateCursor(true);
          AdjustLine(true);
          break;
        case VK_END:
          itrCursor->GoLineEnd();
          UpdateCursor(true);
          AdjustLine(true);
          break;
        default:
          this->TControl::WndProc(Message);
          return;
      }
      break;
    case WM_SETFOCUS:
      if(!KBHook)
        KBHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)ProcessKeyCall, NULL, GetCurrentThreadId());
      SQLEditFocused = this;
      return;
    case WM_KILLFOCUS:
      UnhookWindowsHookEx(KBHook);
      KBHook = NULL;
      return;
    case WM_CHAR:
      switch(Message.WParam)
      {
        case 0x7F: // Process a delete
        case 0x08: // Process a backspace.

          break;
          //case 0x0A: // Process a linefeed.
          //    break;
        case 0x1B: // Process an escape.
          break;
        case 0x09: // Process a tab.
          break;

          //case 0x0D: // Process a carriage return.
          //    break;
        case 0x03:
          Copy();
          break;
        case 0x01:
          SelectAll();
          break;
        case 0x1A:
          {
            int virtkey = GetKeyState(VK_SHIFT);
            Iter * itr;
            Iter * itrbegin;
            if (virtkey & 0x8000)
            {
              itr = buffer->Redo(itrbegin);
            }
            else
            {
              itr = buffer->Undo(itrbegin);
            }
            if(itr != NULL)
            {
              delete itrCursor;
                itrCursor = itr;
                if(buffer->GetLineCount() < GetVisLineCount())
                {
                  delete itrLine;
                    itrLine = buffer->Begin();
                }
              AdjustLine(false);
                if(itrbegin != NULL)
                {
                  parser->ParseFromLine(itrbegin->line, itrbegin->linenum, 0);
                    delete itrbegin;
                }
            }
            UpdateVBar();
            UpdateCursor(false);
            RepaintWindow(true);

#ifdef DEBUG
            /*
               Write("Undone");
               int empty = 0;
               int eno = buffer->CheckIntegrity(empty);
               if(empty)
               Log(String("IntegrityCheck found ")+String(empty)+String(" null strings"));
               if(errno)
               Log(String("IntegrityCheck: ")+String(eno));
               RepaintWindow(true);
               */
#endif

          }
          break;
        case 0x16:
          Paste();
          break;
        default:
          //(Message.WParam < 0x1F)
          //  break;
          changed = itrCursor->line;
          wchar_t * str = new wchar_t[2];
          str[0] = Message.WParam;
          str[1] = '\0';
          if(str[0] == '\r')
          {
            str[0] = '\n';
            linesMovedFrom = GetLineNum(changed)+1;
            if(itrLine->line->prevline == NULL)    //prevent line iter from being moved by insertion
              itrLine->pos = -1;
          }
          //linesMoved = buffer->Insert(itrCursor, str);
          AdjustLine(true);
          Insert(str);
          cursorLeftOffset = itrCursor->GetLeftOffset();
          break;
      }
      break;
    default:
      this->TControl::WndProc(Message);
      return;
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::AdjustLine(bool paint)
{
  int l = GetLineNum(itrCursor->line);

  bool needsFullRepaint = false;
  bool needsHAdjustment = ((cx-HBar->Position) < drawer->GetLinenumWidth()+MARGINS && HBar->Position != 0) || (cx-HBar->Position) > this->Width - MARGINS;

  int movingby = 0;
  if(l >= 0 && l <= KEEP_VIS_TOP && itrLine->line->prevline != NULL)
  {
    for(int i = 0; i < KEEP_VIS_TOP - l; i++)
    {
      if(itrLine->line->prevline)
      {
        itrLine->RevLine();
        movingby--;
      }
    }
  }
  else if(l >= GetVisLineCount()-KEEP_VIS_BOTTOM)
  {
    for(int i = l; i > GetVisLineCount() - KEEP_VIS_BOTTOM; i--)
    {
      if(itrLine->line->nextline)
      {
        itrLine->GoLine();
        movingby++;
      }
    }
  }
  else if(l == -1)
  {
    delete itrLine;
    if(buffer->GetLineCount() <= GetVisLineCount())
    {
      itrLine = buffer->Begin();
    }
    else
    {
      itrLine = itrCursor->Duplicate();
      itrLine->GoLineStart();
      for(int i = 0; i < KEEP_VIS_TOP; i++)
        itrLine->RevLine();
    }
    needsFullRepaint = true;
  }
  //UpdateCursor(paint);
  UpdateVBar();

  if(paint && (needsFullRepaint || needsHAdjustment))
  {
    if(needsHAdjustment)
    {
      int hpos = 0;
        if((cx-HBar->Position) < drawer->GetLinenumWidth()+MARGINS)
          hpos = cx-drawer->GetLinenumWidth()-MARGINS;
        else
          hpos = cx - this->Width + MARGINS;
            if(hpos < 0)
              HBar->Position = 0;
            else
              HBar->Position = hpos;
    }
    RepaintWindow(true);
  }
  else if(paint && movingby != 0)
  {
    if(movingby != 0)
      drawer->DrawMove(0, GetVisLineCount(), -movingby);
    if(movingby < 0)
    {
      Iter * itr = itrLine->Duplicate();
      for(int i = movingby; i <= 0; i++)
      {
        parser->ParseFromLine(itr->line, itr->linenum, 2);
        itr->GoLine();
      }
      parser->Execute();
      delete itr;
    }
    if(movingby > 0)
    {
      Iter * itr = GetLineByNum(GetVisLineCount()-movingby);
      for(int i = movingby; i >= 0; i--)
      {
        parser->ParseFromLine(itr->line, itr->linenum, 2);
        itr->RevLine();
      }
      parser->Execute();
      delete itr;
    }
  }
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void TSQLEdit::CheckIntegrity()
{
  int empty = 0;
  int eno = buffer->CheckIntegrity(empty);
  if(empty)
    Log(String("IntegrityCheck found ")+String(empty)+String(" null strings"));
  if(errno)
    Log(String("IntegrityCheck: ")+String(eno));
}
//---------------------------------------------------------------------------
void TSQLEdit::CheckIterIntegrity(Iter * itr)
{
  Span * sp = itr->word->prev;
  while(sp->string == NULL ||*(sp->string) != '\n')
    sp = sp->prev;
  if((Span*)itr->line != sp)
    Log("IterIntegrityCheck - iter on bad line");
  Iter * tmp = buffer->Begin();
  NSpan * np = itr->line;
  NSpan * dt = tmp->line;
  int j = 1;
  while(np->prevline != NULL)
  {
    if(dt->nextline == NULL)
    {
      Log("IterIntegrityCheck - iter out of buffer - null in forward seek");
      break;
    }
    j++;
    np = np->prevline;
    dt = dt->nextline;
  }
  if(np != tmp->line)
    Log("IterIntegrityCheck - backward seek fail");
  if(dt != itr->line)
    Log("IterIntegrityCheck - forward seek fail");
  if(j != itr->linenum)
    Log("IterIntegrityCheck - wrong linenum");
  delete tmp;
}
#endif DEBUG
//---------------------------------------------------------------------------
void TSQLEdit::UpdateCursor(bool paint)
{

  int lnum = GetLineNum(itrCursor->line);
  if(lnum >= 0)
  {
    int x = 2 + drawer->GetLinenumWidth(), y = Y_OFF + drawer->GetLinesize()*lnum;

    String str = buffer->GetLineTo(itrCursor, true);
    if(!str.IsEmpty() && *(str.LastChar()) != '\0')           //null string is not an empty string... and null char gets drawn if it is only char in string
      x += this->Canvas->TextWidth(str);

    cx = x;
    cy = y;
  }
  cy = (+itrCursor->linenum-itrLine->linenum)*drawer->GetLinesize();
  drawer->UpdateCursor(cx,cy);
  if(paint)
  {
    drawer->Paint();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::MouseDown(TMouseButton Button, Classes::TShiftState Shift, int X, int Y)
{
  this->SetFocus();

  dx = X;
  dy = Y;

  ProcessMouseClear(true, true);
  delete itrCursor;
  itrCursor = XYtoItr(X, Y);
  cursorLeftOffset = itrCursor->GetLeftOffset();

  cx = X;
  cy = Y;

  mx = cx;
  my = cy;

  UpdateCursor(true);
  //drawer->UpdateCursor(cx,cy);
  //drawer->Paint();
  //parser->Execute();

  mouseDown = true;
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::MouseMove(Classes::TShiftState Shift, int X, int Y)
{
  if(!mouseDown)
    return;
  if(!mouseSelect && (abs(X-dx) > 2 || abs(Y-dy) > 2))
    mouseSelect = true;
  if(!mouseSelect)
    return;

#ifndef DEBUG_CURSOR
  ProcessMouseMove(X, Y);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::MouseUp(TMouseButton Button, Classes::TShiftState Shift, int X, int Y)
{
  if(mouseSelect)
    ProcessMouseMove(X, Y);

  mouseDown = false;
  mouseSelect = false;
}
//---------------------------------------------------------------------------
void TSQLEdit::ProcessMouseMove(int &x, int &y)
{
  Iter * oldIter = itrCursorSecond;

  Iter * tmp  = XYtoItr(x, y);
  if(itrCursorSecond != NULL && *tmp == *itrCursorSecond)
  {
    delete tmp;
    return;
  }
  ProcessMouseClear(false, false);
  itrCursorSecond = tmp;
  cursorsInInvOrder = (y < cy || (y == cy && x < cx));
  if(*itrCursor != *itrCursorSecond)
  {
    GetCursor()->MarkupBegin(selectionFormat);
    GetCursorEnd()->MarkupEnd(selectionFormat);
  }

  if(oldIter != NULL) parser->ParseFromLine(oldIter->line, oldIter->linenum, 2);
  if(itrCursor != NULL) parser->ParseFromLine(itrCursor->line, itrCursor->linenum, 2);
  if(itrCursorSecond != NULL) parser->ParseFromLine(itrCursorSecond->line, itrCursorSecond->linenum, 2);

  mx = x;
  my = y;
  delete oldIter;
  parser->Execute();
  /*
     if(!recmsg)
     {
     recmsg = true;
     Application->ProcessMessages();
     parser->Execute();
     recmsg = false;
     }               */
}
//---------------------------------------------------------------------------
void TSQLEdit::ProcessMouseClear(bool redraw, bool deleteiter)
{
  selectionFormat->RemoveAllMarks();
    if(itrCursorSecond)
  {
    if(*itrCursor != *itrCursorSecond && redraw)
    {
      parser->ParseFromLine(GetCursor()->line, GetCursor()->linenum, 2);
      parser->Execute();
    }
    if(deleteiter)
      delete itrCursorSecond;
    cursorsInInvOrder = false;
    itrCursorSecond = NULL;
  }
}
//---------------------------------------------------------------------------
Iter * TSQLEdit::GetCursor()
{
    return cursorsInInvOrder ? itrCursorSecond : itrCursor;
  }
  //---------------------------------------------------------------------------
Iter * TSQLEdit::GetCursorEnd()
{
    return cursorsInInvOrder ? itrCursor : itrCursorSecond;
  }
  //---------------------------------------------------------------------------

Iter * TSQLEdit::XYtoItr(int& x, int& y)
{             /*
                 Iter * itr = GetLineByNum(y/LINESIZE, false);
                 Span * stop = itr->line->nextline;
                 y = (itr->line->nextline->next) ? LINESIZE*(y/LINESIZE) : LINESIZE*GetLineNum(itr->line); //to normalize cursor position; to prevent problems with end of file; and to optimize both
                 int xSum = 2-HBar->Position;
                 int w = this->Canvas->TextWidth(String(itr->word->string).TrimRight());
                 while (xSum + w < x && itr->word != stop)
                 {
                 xSum += w;
                 itr->GoWord();
                 w = this->Canvas->TextWidth(String(itr->word->string));
                 if(!itr->word->next)
                 break;
                 }
                 if(itr->word != stop)
                 {
                 String str = String(itr->word->string); //jump to approximately good place, and then adjust
                 if(str == "\n")
                 str = "";
                 int offset = 0;
                 if(str.Length() > 0 && this->Canvas->TextWidth(str) > 0)
                 {
                 offset = itr->word->length * (x-xSum) / (this->Canvas->TextWidth(str));
                 while(xSum + this->Canvas->TextWidth(str.SubString(0, offset)) > x+5 && offset >= 0)
                 offset--;
                 while(xSum + this->Canvas->TextWidth(str.SubString(0, offset)) < x+5 && offset <= itr->word->length)
                 offset++;
                 offset--;
                 xSum += this->Canvas->TextWidth(str.SubString(0, offset));
                 }
                 if(offset == itr->word->length)
                 itr->GoWord();
                 else
                 {
                 itr->offset = offset;
                 itr->Update();
                 }
                 }
                 x = xSum+HBar->Position;
                 this->Canvas->Unlock();

                 return itr;    */

  Iter * itr = GetLineByNum(y/drawer->GetLinesize(), false);
  int xSum = 2-HBar->Position+drawer->GetLinenumWidth();
  y = (itr->line->nextline->next) ? drawer->GetLinesize()*(y/drawer->GetLinesize()) : drawer->GetLinesize()*GetLineNum(itr->line); //to normalize cursor position; to prevent problems with end of file; and to optimize both
  String line = buffer->GetLine(itr, true);
  int w = Canvas->TextWidth(line);

  if(w+xSum < x || w == 0)
  {
    x = xSum+w;
    itr->GoLineEnd();
    return itr;
  }
  else
  {
    int offset = line.Length() * (x-xSum) / w;
    while(xSum + this->Canvas->TextWidth(line.SubString(0, offset)) > x+4 && offset >= 0)
      offset--;
    while(xSum + this->Canvas->TextWidth(line.SubString(0, offset)) < x+4 && offset <= line.Length())
      offset++;
    offset--;
    xSum += this->Canvas->TextWidth(line.SubString(0, offset));
    x = xSum + HBar->Position;
    itr->GoByOffset(offset);
    return itr;
  }
}
//---------------------------------------------------------------------------
Iter * __fastcall TSQLEdit::GetLineByNum(int no, bool allowEnd)
{
  Iter * itr = itrLine->Duplicate();
  for(int i = 0; i < no; i++)
    if(!itr->GoLine(allowEnd))
      break;
  return itr;
}
//---------------------------------------------------------------------------
//nope, default value does not work... it passes the value EVERY time, no matter whether specified or not
Iter * __fastcall TSQLEdit::GetLineByNum(int no)
{
  return GetLineByNum(no, false);
}
//---------------------------------------------------------------------------
int __fastcall TSQLEdit::GetVisLineCount()
{
  return this->Height/drawer->GetLinesize();
}
//---------------------------------------------------------------------------
int __fastcall TSQLEdit::GetLineCount()
{
  return buffer->GetLineCount();
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void __fastcall TSQLEdit::dbgIter()
{
  Log("cursor pos at "+String(itrCursor->offset)+":"+itrCursor->word->string+" at line "+itrCursor->line->next->string+"; prevline "+(itrCursor->line->prevline != NULL ? itrCursor->line->prevline->next->string : L"")+"+ nextline "+(itrCursor->line->nextline != NULL ? itrCursor->line->nextline->string : L""));
}
#endif
//---------------------------------------------------------------------------
int __fastcall TSQLEdit::GetLineNum(NSpan * line)
{
  NSpan * testline = itrLine->line;
  for(int i = 0, b = GetVisLineCount(); i <= b; i++)
  {
    if(line == testline)
      return i;
    else if(testline->nextline)
    {
      if(testline->nextline->next)
        testline = testline->nextline;
      else
        return -1;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TSQLEdit::DeleteSel(bool allowsync)
{
  int linesMovedFrom = (GetCursorEnd()->linenum-itrLine->linenum)+1;

  int linesMoved = -buffer->Delete(GetCursor(), GetCursorEnd());

  bool needRepaint = GetCursor()->linenum < itrLine->linenum;

  NSpan* changed = itrCursor->line;
  selectionFormat->RemoveAllMarks();
  delete itrCursorSecond;
  cursorsInInvOrder = false;
  itrCursorSecond = NULL;

  if(linesMoved != 0)
    needRepaint |= drawer->UpdateLinenumWidth(buffer->GetLineCount());

  if(needRepaint) //means that line iterator has been moved as well
  {
    delete itrLine;
    itrLine = itrCursor->Duplicate();
    AdjustLine(false);
    RepaintWindow(true);
  }
  else
  {
    ProcessChange(linesMovedFrom < 0 ? 0 : linesMovedFrom, linesMoved, changed);
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::Insert(wchar_t * text)
{
  if(itrCursorSecond)
    DeleteSel(false);

  parser->ParseFromLine(itrCursor->line, itrCursor->linenum, 0);

  NSpan* changed = itrCursor->line;
  int linesMovedFrom = GetLineNum(itrCursor->line)+1;
  int linesMoved = buffer->Insert(itrCursor, text);
  itrLine->GoLineStart();
  //itrCursor->linenum += linesMoved;

  if(linesMoved != 0 && drawer->UpdateLinenumWidth(buffer->GetLineCount()))
    RepaintWindow(true);
  else
    ProcessChange(linesMovedFrom, linesMoved, changed);
}
//---------------------------------------------------------------------------
void TSQLEdit::ProcessChange(int linesMovedFrom, int linesMoved, NSpan * changed)
{
  cy += drawer->GetLinesize() *linesMoved;
  my += drawer->GetLinesize() *linesMoved;
  dy += drawer->GetLinesize() *linesMoved;
  if(linesMoved != 0 && abs(linesMoved) < GetVisLineCount())
  {
    drawer->DrawMove(linesMovedFrom, GetVisLineCount(), linesMoved);
    if(linesMoved < 0)
    {
      //scroll down (move image up)
      int barcorrection = HBar->Visible ? HBar->Height/drawer->GetLinesize()+1 : 1;
      Iter * it = GetLineByNum(GetVisLineCount()+linesMoved-barcorrection);
      for(int i = linesMoved-1; i < 0 && it->line->nextline; i++, it->GoLine())
        parser->ParseFromLine(it->line, it->linenum, 2);
      delete it;
    }
    if(linesMoved > 0)
    {
      //scroll up (move image down)
      Iter * it = itrLine->Duplicate();
      for(int i = 0; i < linesMoved && it->line->nextline && i < GetVisLineCount(); i++, it->GoLine())
        parser->ParseFromLine(it->line, it->linenum,2);
      delete it;
    }
    UpdateVBar();
  }
  else if(linesMoved != 0)
  {
    RepaintWindow(true);
      UpdateVBar();
  }


  if(changed)
  {
    UpdateCursor(false);
    short linenum = GetLineNum(changed);
    parser->ParseFromLine(changed, itrLine->linenum+linenum, linenum >= 0 ? 2 : 1);
    parser->ParseFromLine(itrCursor->line, itrCursor->linenum, linenum >= 0 ? 2 : 1);
  }
  parser->Execute();
  /*
     if(!recmsg)
     {
     recmsg = true;
     Application->ProcessMessages();
     parser->Execute();
     recmsg = false;
     }        */
}
//---------------------------------------------------------------------------
void TSQLEdit::UpdateVBar()
{
  if(buffer->GetLineCount() >= GetVisLineCount() - 5)
  {
    if(!VBar->Visible)
    {
      VBar->Show();
      drawer->DrawResize(this->Width, this->Height);
    }
    VBar->PageSize = GetVisLineCount();
    VBar->LargeChange = GetVisLineCount()-5;
    VBar->Min = 1;
    VBar->Max = buffer->GetLineCount()+GetVisLineCount()-2;
    VBar->Position = itrLine->linenum;
  }
  else if(VBar->Visible)
  {
    VBar->Hide();
    drawer->DrawResize(this->Width, this->Height);
    RepaintWindow(true);
    return;
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::UpdateHBar()
{
  if(drawer->HMax > this->Width)
  {
    HBar->Max = drawer->HMax;
    HBar->LargeChange = this->Width - 50;
    HBar->SmallChange = 50;
    HBar->PageSize = this->Width;
    if(!HBar->Visible)
    {
      HBar->Show();
      drawer->DrawResize(this->Width, this->Height);
    }
  }
  else if(HBar->Visible)
  {
    HBar->Position = 0;
    HBar->Hide();
    drawer->DrawResize(this->Width, this->Height);
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::Copy()
{
  if(itrCursorSecond)
  {
    wchar_t * str = buffer->GetText(GetCursor(), GetCursorEnd());
    clipboard->SetTextBuf(str);
    delete[] str;
  }
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::OnVScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos)
{
  //if(ScrollCode != scEndScroll && (clock()-lastHBarUpdate)/((double)CLOCKS_PER_SEC)<0.1)    //do not update too often - on big files would be a problem...
  //  return;

  /*     //from tests it seems, that rarely happens that there are 2 scroll events in message queue
         if(lastscrollmessage != -1)
         {
         lastscrollmessage = ScrollPos;
         return;
         }
         else
         {
         lastscrollmessage = ScrollPos;
         Application->ProcessMessages();
         } */

  if(abs(ScrollPos-itrLine->linenum) < GetVisLineCount())
  {
    Scroll(itrLine->linenum-ScrollPos);
  }
  else
  {

    cy += drawer->GetLinesize() * (itrLine->linenum - ScrollPos);
    my += drawer->GetLinesize() * (itrLine->linenum - ScrollPos);
    dy += drawer->GetLinesize() * (itrLine->linenum - ScrollPos);
    if(ScrollPos < itrLine->linenum)
    {
      if(ScrollPos > itrLine->linenum/2 && ScrollPos > 100)
        itrLine->GoToLine(ScrollPos);
      else
      {
        delete itrLine;
        itrLine = buffer->Begin();
        itrLine->GoToLine(ScrollPos);
      }
    }
    else
    {
      itrLine->GoToLine(ScrollPos);
    }
    ScrollPos = -1;
    drawer->UpdateCursor(cx, cy);
    RepaintWindow(true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::OnHScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos)
{
  if(ScrollCode == scEndScroll)
  {
    RepaintWindow(true);
  }
}
//---------------------------------------------------------------------------
int __fastcall TSQLEdit::GetActLine()
{
  return itrLine->linenum;
}
//---------------------------------------------------------------------------
void TSQLEdit::Paste()
{
  int basesize = 128;
  int memused = basesize;
  wchar_t * buf = NULL;
  //oh funny, tclipboard does not provide info about size of data stored in memory...
  while(memused >= basesize-1)
  {
    basesize *= 2;
    if(buf)
      delete[] buf;
    buf = new wchar_t[basesize];
    memused = clipboard->GetTextBuf(buf, basesize);
  }
  Insert(buf);
  AdjustLine(true);
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::OnResizeCallback(TObject * Sender)
{
  UpdateHBar();
  drawer->DrawResize(this->Width, this->Height);

}
//---------------------------------------------------------------------------
bool __fastcall TSQLEdit::GetLineFirst(NSpan * line)
{
  return line == itrLine->line;
}
//---------------------------------------------------------------------------
void TSQLEdit::SelectAll()
{
  this->ProcessMouseClear(false, true);
  delete itrCursor;
  itrCursor = buffer->Begin();
  itrCursorSecond = buffer->End();
  itrCursor->MarkupBegin(this->selectionFormat);
  itrLine->MarkupBegin(this->selectionFormat);
  parser->ParseFromLine(itrCursor->line, itrCursor->linenum, 0);
  parser->ParseFromLine(itrLine->line, itrLine->linenum, 2);
  UpdateCursor(false);
  parser->Execute();
}
//---------------------------------------------------------------------------
void TSQLEdit::LoadFile(char * filename)
{
  buffer->SimpleLoadFile(filename);
  delete itrCursor;
  delete itrLine;
  itrCursor = buffer->Begin();
  itrLine = buffer->Begin();
  parser->InvalidateAll();
  parser->ParseFromLine(buffer->FirstLine(), 1, 0);
  UpdateCursor(false);
  UpdateVBar();
  RepaintWindow(true);
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::SetFontsize(int size)
{
  drawer->SetFontsize(size);
}
//---------------------------------------------------------------------------
int __fastcall TSQLEdit::GetFontsize()
{
  return drawer->GetFontsize();
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::SetLinenumsEnabled(bool enabled)
{
  drawer->SetLinenumsEnabled(enabled);
}
//---------------------------------------------------------------------------
bool __fastcall TSQLEdit::GetLinenumsEnabled()
{
  return drawer->GetLinenumsEnabled();
}
//---------------------------------------------------------------------------

#ifdef DEBUG
void __fastcall TSQLEdit::Log(String str)
{
  if(dbgLog != NULL)
    this->dbgLog->Lines->Add(str);
}
#endif
//---------------------------------------------------------------------------
#ifdef DEBUG
void TSQLEdit::Write(AnsiString message)
{
#ifdef DEBUG_LOGGING
  myfile << message.c_str() << std::endl;
#endif
}
#endif
