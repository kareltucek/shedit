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

  this->DoubleBuffered = true;

  this->Color = clWhite;

  scrolldelta = 0;
  cursorLeftOffset = 0;

  recmsg = false;

  selectionFormat = new Format(NULL, new TColor(clGray));
  cursorsInInvOrder = false;

  clipboard = Clipboard();

  bufferChanged = CreateEvent(NULL, true, false, NULL);
  bufferMutex = CreateMutex(NULL, false, NULL);
  drawerCanvasMutex = CreateMutex(NULL, false, NULL);
  drawerQueueMutex = CreateMutex(NULL, false, NULL);
  drawerTaskPending = CreateEvent(NULL, true, false, NULL);
#ifdef DEBUG
  DWORD tst;
  if(!GetHandleInformation(bufferMutex, &tst))
    Write(String("gethandleinfo error ")+String(GetLastError()));
#endif

  drawer = new Drawer(this->Canvas, this, drawerCanvasMutex, drawerQueueMutex, drawerTaskPending) ;
  parser = new Parser(this, drawer, bufferChanged, bufferMutex, drawerCanvasMutex, drawerQueueMutex, drawerTaskPending);
  parser->SetLangDef(new LanguageDefinitionSQL());

  if(!ComponentState.Contains(csDesigning))
  {
    drawer->Start() ;
    parser->Start() ;
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
  /*
     Iter * itrCpy = itrLine->Duplicate();
     Canvas->Font->Name = "Courier New";
     Canvas->Font->Size = FONTSIZE;
     bool firstCursor = false;
     for(int i = 0, count = GetVisLineCount(); i < count; i++)
     {
     int linewidth = 2;
     if(!itrCpy->line->nextline)
     break;
     itrCpy->GoWord();
     while(*(itrCpy->word->string) != '\n')
     {
     String text = String(itrCpy->word->string);
     int w = Canvas->TextWidth(text);
     Canvas->Rectangle(linewidth, LINESIZE*i, linewidth+w, LINESIZE*(i+1));
     Canvas->TextOutW(linewidth, LINESIZE*i, text);
     if(itrCpy->word == itrCursor->word && !firstCursor)
     {
     curX = linewidth+Canvas->TextWidth(text.SubString(0, itrCursor->offset));
     curY = LINESIZE*i;
     firstCursor = true;
     }
     linewidth += w;
     itrCpy->GoWord();
     }
     if(itrCpy->word == itrCursor->word && !firstCursor)
     {
     curX = linewidth;
     curY = LINESIZE*i;
     firstCursor = true;
     }
     Canvas->Brush->Color = clWhite;
     Canvas->Pen->Color = clWhite;
     Canvas->Rectangle(linewidth, LINESIZE*i, this->Width, LINESIZE*(i+1));
     }
     Canvas->Pen->Color = clBlack;
     Canvas->MoveTo(curX, curY);
     Canvas->LineTo(curX, curY+LINESIZE);
     delete itrCpy;
     */
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::PaintWindow(HDC DC)
{
  /*
     Canvas->Rectangle(-1, -1, 1+this->Width, 1+this->Height);
     Paint();*/
  RepaintWindow(false);
  //test->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::RepaintWindow(bool force)
{
  if(!DEBUG_REPAINT || force)
  {
    WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
    NSpan * line = itrLine->line;
    for(int i = GetVisLineCount(); i > 0; i--)
    {
      parser->ParseFromLine(line, 2);
      if(line->nextline && line->nextline->nextline)
        line = line->nextline;
      else
        break;
    }
    ReleaseMutex(bufferMutex);
    SetEvent(bufferChanged);

  }
  this->SetAutoSize(false);
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
  if((lParam & 2147483648) == 0)
  {
    switch(wParam)
    {
      case VK_LEFT:
        itrCursor->RevChar();
        AdjustLine();
        cursorLeftOffset = itrCursor->GetLeftOffset();
        trap = true;
        break;
      case VK_RIGHT:
        itrCursor->GoChar();
        AdjustLine();
        cursorLeftOffset = itrCursor->GetLeftOffset();
        trap = true;
        break;
      case VK_UP:
        itrCursor->RevLine();
        itrCursor->GoBy(cursorLeftOffset);
        AdjustLine();
        trap = true;
        break;
      case VK_DOWN:
        int tmp = itrCursor->GetLeftOffset();
        itrCursor->GoLine();
        itrCursor->GoBy(cursorLeftOffset);
        AdjustLine();
        trap = true;
        break;
    }
  }
  if(trap)
  {
    UpdateCursor();
    return 1;
  }
  else
    return 0;
}
//---------------------------------------------------------------------------
void TSQLEdit::Scroll(int by)
{
  if(by == 0)
    return;
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  int scrolled = 0;
  if(by < GetVisLineCount())
  {
    while(by >= 1 || by <= -1)
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
    UpdateCursor();
  }
  ReleaseMutex(bufferMutex);

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
        scrolldelta += (short)HIWORD(Message.WParam);
        int scrolled = scrolldelta/120;
        scrolldelta = scrolldelta-scrolled*120;
        Scroll(scrolled*SCROLL_STEP);
      }
      return;
    case WM_KEYDOWN:
      switch(Message.WParam)
      {
        case VK_DELETE:
        case VK_BACK:
          WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
          if(!itrCursorSecond)
          {
            if(Message.WParam == VK_BACK && !(itrCursor->word->prev->prev || (itrCursor->word->prev && itrCursor->offset > 0)))  //have to stop after the head, not on it
            {
              ReleaseMutex(bufferMutex);
              break;
            }
            if(Message.WParam == VK_DELETE && !(itrCursor->word->next))  //have to stop after the head, not on it
            {
              ReleaseMutex(bufferMutex);
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
          AdjustLine();
#ifdef DEBUG
          {
            Write("deleted");
            int empty = 0;
            int eno = buffer->CheckIntegrity(empty);
            if(empty)
              Log(String("IntegrityCheck found ")+String(empty)+String(" null strings"));
            if(errno)
              Log(String("IntegrityCheck: ")+String(eno));
          }
#endif
          ReleaseMutex(bufferMutex);
          break;
        case VK_F2:
          WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
          buffer->LoadFile(L"test.txt");
          UpdateVBar();
          ReleaseMutex(bufferMutex);
          UpdateCursor();
          RepaintWindow(true);
          return;
        case VK_F5:
          RepaintWindow(true);
          return;
        case VK_F6:
          {
            int justbreaksomewhere = 666;
          }
          return;
        case VK_PRIOR:
        case VK_NEXT:
          for(int i = GetVisLineCount(); i > 0; i--)
          {
            if(Message.WParam == VK_PRIOR)
              itrLine->RevLine();
            else
              itrLine->GoLine();
          }
          UpdateCursor();
          UpdateVBar();
          RepaintWindow(true);
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
      WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
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
        case 0x1A:
          {
            int virtkey = GetKeyState(VK_SHIFT);
            if (virtkey & 0x8000)
              buffer->Redo();
            else
              buffer->Undo();
            UpdateCursor();
            RepaintWindow(true);

#ifdef DEBUG
            Write("Undone");
            int empty = 0;
            int eno = buffer->CheckIntegrity(empty);
            if(empty)
              Log(String("IntegrityCheck found ")+String(empty)+String(" null strings"));
            if(errno)
              Log(String("IntegrityCheck: ")+String(eno));
            RepaintWindow(true);
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
          }
          //linesMoved = buffer->Insert(itrCursor, str);
          AdjustLine();
          Insert(str);
          cursorLeftOffset = itrCursor->GetLeftOffset();
          break;
      }
      ReleaseMutex(bufferMutex);
      break;
    default:
      this->TControl::WndProc(Message);
      return;
  }           /*
                 if(linesMoved != 0)
                 drawer->Draw(new DrawTaskMove(linesMovedFrom, GetVisLineCount(), linesMoved));
                 if(changed)
                 {
                 UpdateCursor(false);
                 short linenum = GetLineNum(changed);
                 parser->ParseFromLine(changed, linenum >= 0 ? 2 : 1);
                 parser->ParseFromLine(itrCursor->line, linenum >= 0 ? 2 : 1);
                 Iter * it = GetLineByNum(GetVisLineCount()+linesMoved);
                 for(int i = linesMoved; i < 0 && it->line->nextline; i++, it->GoLine())
                 parser->ParseFromLine(it->line, 2);
                 delete it;
                 }
                 if(!recmsg)
                 {
                 recmsg = true;
                 Application->ProcessMessages();
                 SetEvent(bufferChanged);
                 recmsg = false;
                 }             */
}
//---------------------------------------------------------------------------
void TSQLEdit::AdjustLine()
{
  int l = GetLineNum(itrCursor->line);
  if(l > 4 && l < GetVisLineCount() - 4)
    return;
  int movingby = 0;
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  if(l >= 0 && l <= 4)
  {
    for(int i = 0; i < 4 - l; i++)
    {
      if(itrLine->line->prevline)
      {
        itrLine->RevLine();
        movingby--;
      }
    }
  }
  else if(l >= GetVisLineCount()-4)
  {
    for(int i = l; i > GetVisLineCount() - 4; i--)
    {
      if(itrLine->line->nextline)
      {
        itrLine->GoLine();
        movingby++;
      }
    }
  }
  else
  {
    delete itrLine;
    itrLine = itrCursor->Duplicate();
    for(int i = 0; i < 4; i++)
      itrLine->RevLine();
    RepaintWindow(true);
    ReleaseMutex(bufferMutex);
    UpdateVBar();
    return;
  }
  WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  UpdateCursor();
  drawer->Draw(new DrawTaskMove(movingby < 0 ? -movingby : 0, GetVisLineCount(), -movingby));
  ReleaseMutex(drawerQueueMutex);
  UpdateVBar();
  if(movingby < 0)
  {
    Iter * itr = itrLine->Duplicate();
    for(int i = movingby; i <= 0; i++)
    {
      parser->ParseFromLine(itr->line, 2);
      itr->GoLine();
    }
    SetEvent(bufferChanged);
    delete itr;
  }
  if(movingby > 0)
  {
    Iter * itr = GetLineByNum(GetVisLineCount()-movingby);
    for(int i = movingby; i >= 0; i--)
    {
      parser->ParseFromLine(itr->line, 2);
      itr->RevLine();
    }
    SetEvent(bufferChanged);
    delete itr;
  }
  ReleaseMutex(bufferMutex);
}
//---------------------------------------------------------------------------
void TSQLEdit::UpdateCursor()
{
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);

  this->Canvas->Lock();
  int lnum = GetLineNum(itrCursor->line);
  int x = 2, y = Y_OFF + LINESIZE*lnum;
  if(lnum >= 0 && itrCursor->line->nextline && itrCursor->line->next != (Span*)itrCursor->line->nextline)
  {
    for(Span * word = itrCursor->line->next; word != itrCursor->word; word = word->next)
    {
      x += this->Canvas->TextWidth(String(word->string));
    }
    x += this->Canvas->TextWidth(String(itrCursor->word->string).SubString(0, itrCursor->offset));
  }
  WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  drawer->Draw(new DrawTaskCursor(x,y));
  ReleaseMutex(drawerQueueMutex);
  this->Canvas->Unlock();

  cx = x;
  cy = y;
  ReleaseMutex(drawerCanvasMutex);
  ReleaseMutex(bufferMutex);
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::MouseDown(TMouseButton Button, Classes::TShiftState Shift, int X, int Y)
{
  this->SetFocus();

  dx = X;
  dy = Y;

  ProcessMouseClear();
  delete itrCursor;
  itrCursor = XYtoItr(X, Y);
  cursorLeftOffset = itrCursor->GetLeftOffset();

  cx = X;
  cy = Y;

  SetEvent(bufferChanged);
  WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  drawer->Draw(new DrawTaskCursor(X, Y));
  ReleaseMutex(drawerQueueMutex);

  mouseDown = true;
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::MouseMove(Classes::TShiftState Shift, int X, int Y)
{
  if(!mouseDown)
    return;
  if(!mouseSelect && (abs(X-dx) > 2 || abs(X-dx) > 2))
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
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  NSpan * ch = GetCursor() != NULL ? GetCursor()->line : NULL;
  NSpan * ch2 = GetCursorEnd() != NULL ? GetCursorEnd()->line : NULL;

  ProcessMouseClear();
  itrCursorSecond = XYtoItr(x, y);
  bool lastInvOrder = cursorsInInvOrder;
  cursorsInInvOrder = (y < cy || (y == cy && x < cx));
  if(*itrCursor != *itrCursorSecond)
  {
    GetCursor()->MarkupBegin(selectionFormat);
    GetCursorEnd()->MarkupEnd(selectionFormat);
  }
  if(*itrCursor != *itrCursorSecond)
  {
    if(cursorsInInvOrder)
    {
      parser->ParseFromLine(GetCursor()->line, 2);
      if(ch) parser->ParseFromLine(ch, 2);
      if(GetCursorEnd()) parser->ParseFromLine(GetCursorEnd()->line, 2);
      if(ch2) parser->ParseFromLine(ch2, 2);
    }
    else
    {
      if(ch) parser->ParseFromLine(ch, 2);
      parser->ParseFromLine(GetCursor()->line, 2);
      if(GetCursorEnd()) parser->ParseFromLine(GetCursorEnd()->line, 2);
      if(ch2) parser->ParseFromLine(ch2, 2);
    }
  }
  if(!recmsg)
  {
    recmsg = true;
    Application->ProcessMessages();
    SetEvent(bufferChanged);
    recmsg = false;
  }
  ReleaseMutex(bufferMutex);
}
//---------------------------------------------------------------------------
void TSQLEdit::ProcessMouseClear()
{
      WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  selectionFormat->RemoveAllMarks();
    if(itrCursorSecond)
  {
    if(*itrCursor != *itrCursorSecond)
      parser->ParseFromLine(GetCursor()->line, 2);
    delete itrCursorSecond;
    cursorsInInvOrder = false;
    itrCursorSecond = NULL;
  }
  ReleaseMutex(bufferMutex);
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
{
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);


  Iter * itr = GetLineByNum(y/LINESIZE, false);
  this->Canvas->Lock();
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

  ReleaseMutex(bufferMutex);
  ReleaseMutex(drawerCanvasMutex);
  return itr;
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
  return this->Height/LINESIZE;
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
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  int linesMovedFrom = GetLineNum(GetCursorEnd()->line)+1;

  int linesMoved = -buffer->Delete(GetCursor(), GetCursorEnd());

  NSpan* changed = itrCursor->line;
  selectionFormat->RemoveAllMarks();
  delete itrCursorSecond;
  cursorsInInvOrder = false;
  itrCursorSecond = NULL;

  ProcessChange(linesMovedFrom, linesMoved, changed);
  ReleaseMutex(bufferMutex);
}
//---------------------------------------------------------------------------
void TSQLEdit::Insert(wchar_t * text)
{
  WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
  if(itrCursorSecond)
    DeleteSel(false);
  NSpan* changed = itrCursor->line;
  int linesMovedFrom = GetLineNum(itrCursor->line)+1;
  int linesMoved = buffer->Insert(itrCursor, text);
  itrCursor->linenum += linesMoved;

  ProcessChange(linesMovedFrom, linesMoved, changed);
  ReleaseMutex(bufferMutex);
}
//---------------------------------------------------------------------------
void TSQLEdit::ProcessChange(int linesMovedFrom, int linesMoved, NSpan * changed)
{
  if(linesMoved != 0)
  {
    drawer->Draw(new DrawTaskMove(linesMovedFrom, GetVisLineCount(), linesMoved));
    if(linesMoved < 0)
    {
      //scroll down (move image up)
      Iter * it = GetLineByNum(GetVisLineCount()+linesMoved-1);
      for(int i = linesMoved-1; i < 0 && it->line->nextline; i++, it->GoLine())
        parser->ParseFromLine(it->line, 2);
      delete it;
    }
    if(linesMoved > 0)
    {
      //scroll up (move image down)
      Iter * it = itrLine->Duplicate();
      for(int i = 0; i < linesMoved && it->line->nextline; i++, it->GoLine())
        parser->ParseFromLine(it->line, 2);
      delete it;
    }
    UpdateVBar();
  }
  if(changed)
  {
    UpdateCursor();
    short linenum = GetLineNum(changed);
    parser->ParseFromLine(changed, linenum >= 0 ? 2 : 1);
    parser->ParseFromLine(itrCursor->line, linenum >= 0 ? 2 : 1);
  }
  if(!recmsg)
  {
    recmsg = true;
    Application->ProcessMessages();
    SetEvent(bufferChanged);
    recmsg = false;
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::UpdateVBar()
{
  if(buffer->linecount > GetVisLineCount())
  {
    if(!VBar->Visible)
      VBar->Show();
    VBar->PageSize = GetVisLineCount();
    VBar->LargeChange = GetVisLineCount()-5;
    VBar->Max = buffer->linecount+VBar->LargeChange;
    VBar->Position = itrLine->linenum;
  }
  else if(VBar->Visible)
  {
    VBar->Hide();
    RepaintWindow(true);
    return;
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::UpdateHBar()
{
  if(drawer->HMax > this->Width)
  {
    HBar->PageSize = this->Width;
    HBar->LargeChange = this->Width - 50;
    HBar->SmallChange = 50;
    HBar->Max = drawer->HMax;
    if(!HBar->Visible)
      HBar->Show();
  }
  else if(HBar->Visible)
  {
    HBar->Position = 0;
    HBar->Hide();
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
  if(ScrollCode != scEndScroll && (clock()-lastHBarUpdate)/((double)CLOCKS_PER_SEC)<0.05)    //do not update too often - on big files would be problem...
    return;

  lastHBarUpdate = clock();
  if(abs(ScrollPos-itrLine->linenum) < GetVisLineCount())
  {
    Scroll(itrLine->linenum-ScrollPos);
  }
  else
  {
    WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
    if(ScrollPos < itrLine->linenum)
    {
      if(ScrollPos > itrLine->linenum/2 && ScrollPos > 100)
      {
        while(itrLine->linenum > ScrollPos) itrLine->RevLine();
      }
      else
      {
        delete itrLine;
        itrLine = buffer->Begin();
        while(itrLine->linenum < ScrollPos)
          itrLine->GoLine();
      }
    }
    else
    {
      bool last = itrLine->line->parserState.parsed;
      while(itrLine->linenum < ScrollPos)
      {
        if(itrLine->line->parserState.parsed != last)
        {
          if( !itrLine->line->parserState.parsed )
            parser->ParseFromLine(itrLine->line, 0);
          last = !last;
        }
        itrLine->GoLine();
      }
    }
    RepaintWindow(true);
    ReleaseMutex(bufferMutex);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::OnHScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos)
{
  if(ScrollCode == scEndScroll)
  {
    UpdateHBar();
    RepaintWindow(true);
  }
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
  AdjustLine();
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::OnResizeCallback(TObject * Sender)
{
  UpdateHBar();
}
//---------------------------------------------------------------------------
bool __fastcall TSQLEdit::GetLineFirst(NSpan * line)
{
  return line == itrLine->line;
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
  myfile << message.c_str() << std::endl;
}
#endif
