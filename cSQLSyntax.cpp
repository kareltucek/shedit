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
  buffer = new Buffer();
  itrLine = buffer->First();
  itrCursor = buffer->End();
  itrCursorSecond = NULL;

  recmsg = false;

  selectionFormat = new Format(NULL, new TColor(clGray));
  cursorsInInvOrder = false;

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
        trap = true;
        break;
      case VK_RIGHT:
        itrCursor->GoChar();
        trap = true;
        break;
      case VK_UP:
        itrCursor->RevLine();
        trap = true;
        break;
      case VK_DOWN:
        itrCursor->GoLine();
        trap = true;
        break;
    }
  }
  if(trap)
  {
    UpdateCursor(true);
    return 1;
  }
  else
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TSQLEdit::WndProc(Messages::TMessage &Message)
{
#ifdef DEBUG
  Write("received msg local");
#endif
  bool processed = false;
  NSpan* changed;
  Iter * itr;
  int linesMoved;
  switch(Message.Msg)
  {        /*
              case WM_KEYDOWN:
              switch(Message.WParam)
              {
              case VK_LEFT:
              itrCursor->GoChar();
              break;
              case VK_RIGHT:
              itrCursor->RevChar();
              break;
              case VK_UP:
              itrCursor->RevLine();
              break;
              case VK_DOWN:
              itrCursor->GoChar();
              break;
              default:
              this->TControl::WndProc(Message);
              break;
              }
              break;   */
    case WM_KEYDOWN:
      switch(Message.WParam)
      {
      case VK_F2:
        WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
        buffer->LoadFile(L"test.txt");
        ReleaseMutex(bufferMutex);
        UpdateCursor(true);
        RepaintWindow(true);
        break;
      case VK_F5:
        RepaintWindow(true);
        break;
      default:
        this->TControl::WndProc(Message);
        break;
      }
      break;
    case WM_SETFOCUS:
      if(!KBHook)
        KBHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)ProcessKeyCall, NULL, GetCurrentThreadId());
      SQLEditFocused = this;
      break;
    case WM_KILLFOCUS:
      UnhookWindowsHookEx(KBHook);
      KBHook = NULL;
      break;
    case WM_CHAR:

#ifdef DEBUG
      Write("new change");
#endif
      WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);

#ifdef DEBUG
      Write("parser is idle and buffer idle is reset");
#endif
      linesMoved = 0;
      changed = NULL;
      switch(Message.WParam)
      {
        case 0x08: // Process a backspace.
          if(itrCursor->word->prev->prev || (itrCursor->word->prev && itrCursor->offset > 0))  //have to stop after the head, not on it
          {
            itr = itrCursor->Duplicate();
            itr->RevChar();
            if(itr->word->prev)
              linesMoved = -buffer->Delete(itr, itrCursor);      // '-' is not a typo!
            changed = itrCursor->line;
            delete itr;
            this->Paint();
          }
          break;
          //case 0x0A: // Process a linefeed.
          //    break;
        case 0x1B: // Process an escape.
          break;
        case 0x09: // Process a tab.
          break;
        case 0x7F: // Process a delete
          break;

          //case 0x0D: // Process a carriage return.
          //    break;
        default:

#ifdef DEBUG
          Write(AnsiString(Message.WParam));
#endif
          changed = itrCursor->line;
          wchar_t * str = new wchar_t[2];
          str[0] = Message.WParam;
          str[1] = '\0';
          if(str[0] == '\r')
            str[0] = '\n';
          linesMoved = buffer->Insert(itrCursor, str);
#ifdef DEBUG
          dbgIter();
#endif
          break;
      }
      if(linesMoved != 0)
        drawer->Draw(new DrawTaskMove(GetLineNum(changed)+1, GetVisLineCount(), linesMoved));
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

#ifdef DEBUG

      Write("going to sync");
#endif
      if(!recmsg)
      {
        recmsg = true;
        Application->ProcessMessages();
        SetEvent(bufferChanged);
        recmsg = false;
      }
      ReleaseMutex(bufferMutex);

#ifdef DEBUG
      Write("finished");
#endif
      break;
    default:
      this->TControl::WndProc(Message);
      break;
  }
}
//---------------------------------------------------------------------------
void TSQLEdit::UpdateCursor(bool sync)
{
  if(sync)
  {
    WaitForSingleObject(bufferMutex, WAIT_TIMEOUT_TIME);
    WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);
  }

  this->Canvas->Lock();
  int x = 2, y = Y_OFF + LINESIZE*GetLineNum(itrCursor->line);
  if(itrCursor->line->nextline && itrCursor->line->next != (Span*)itrCursor->line->nextline)
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

  if(sync)
  {
    ReleaseMutex(bufferMutex);
    ReleaseMutex(drawerCanvasMutex);
  }
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
  cursorsInInvOrder = (y < cy || (y == cy && x < cx));
  if(*itrCursor != *itrCursorSecond)
  {
    GetCursor()->MarkupBegin(selectionFormat);
    GetCursorEnd()->MarkupEnd(selectionFormat);
  }
  if(*itrCursor != *itrCursorSecond)
  {
    if(ch) parser->ParseFromLine(ch, 2);
    parser->ParseFromLine(GetCursor()->line, 2);
    if(GetCursorEnd()) parser->ParseFromLine(GetCursorEnd()->line, 2);
    if(ch2) parser->ParseFromLine(ch2, 2);
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
  int xSum = 2;
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
    String str = String(itr->word->string).Trim(); //jump to approximately good place, and then adjust
    int offset = 0;
    if(str.Length() > 0)
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
  x = xSum;
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
        return i;
    }
  }
  return -1;
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
