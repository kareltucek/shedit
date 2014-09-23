//---------------------------------------------------------------------------
#include <vcl.h>

#pragma hdrstop

#include "cSHEdit.h"
#include "uBuffer.h"
#include "uIter.h"
#include "uParser.h"
#include "uDrawer.h"
//include "fSearchBar.h"
#include "uLanguageDefinition.h"
#include "uLanguageDefinitionSQL.h"
#include <windows.h>
#pragma package(smart_init)
//---------------------------------------------------------------------------
// ValidCtrCheck is used to assure that the components created do not have
// any pure virtual functions.
//

static inline void ValidCtrCheck(TSHEdit *)
{
  new SHEdit::TSHEdit(NULL);
}
//---------------------------------------------------------------------------
namespace Cshedit
{
  void __fastcall PACKAGE Register()
  {
    TComponentClass classes[1] = {__classid(SHEdit::TSHEdit)};
    RegisterComponents(L"Samples", classes, 0);
  }
}
//---------------------------------------------------------------------------

using namespace SHEdit;

#ifdef _DEBUG_LOGGING
#include <fstream>
std::wofstream myfile;
#endif

//---------------------------------------------------------------------------

TSHEdit * TSHEditFocused; //callback musi jit na statickou metodu...

//---------------------------------------------------------------------------
  __fastcall TSHEdit::TSHEdit(TComponent* Owner)
: TCustomControl(Owner)
{
#ifdef _DEBUG_LOGGING
  if(!myfile.is_open())
    myfile.open("main.txt", ios::out );
#endif
  msgLock = false;

  VBar = new TScrollBar(this);
  VBar->ParentWindow = this->ParentWindow;
  VBar->SetParentComponent(this);
  VBar->Align = alRight;
  VBar->Kind = sbVertical;
  VBar->Width = 15;
  VBar->SmallChange = MAX_SCROLL_STEP;
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


  timer = new TTimer(this);
  timer->Enabled = false;
  timer->Interval = 1000;
  timer->OnTimer = OnTimer;

  OnResize = OnResizeCallback;

  buffer = new Buffer();
  itrLine = buffer->begin();
  itrCursor = buffer->end();
  itrCursorSecond = Iter();

  //this->TWinControl::OnKeyDown = KeyDownHandler;
  //this->TWinControl::OnKeyPress = KeyPressHandler;
  //this->TWinControl::OnKeyUp = KeyUpHandler;

  //this->TWinControl::OnEnter = OnEnterHandler; just a test

  //this->DoubleBuffered = true;

  this->Color = clWhite;

  scrolldelta = 0;
  scrolldeltafontsize = 0;
  cursorLeftOffset = 0;
  mouseDoubleClickFlag = false;

  recmsg = false;
  maxScrollStep = MAX_SCROLL_STEP;

  selColor = clHighlight;
  selFColor = clHighlightText;
  searchColor = clYellow;
  selectionFormat = new Format(&selFColor, &selColor);
  searchFormat = new Format(NULL, &searchColor);

  clipboard = Clipboard();

  Cursor = crIBeam;

  drawer = new Drawer(this->Canvas, this);
  parser = new Parser(this, drawer);
  parser->SetLangDef(new LanguageDefinition());

  drawer->UpdateLinenumWidth(100);

  if(!ComponentState.Contains(csDesigning))
  {
    //drawer->Start() ;
    //parser->Start() ;
  }
}

//---------------------------------------------------------------------------
__fastcall TSHEdit::~TSHEdit()
{
  UnhookWindowsHookEx(KBHook);
  if(this == TSHEditFocused)
    TSHEditFocused = NULL;
  itrLine.Invalidate();
  itrCursor.Invalidate();
  itrCursorSecond.Invalidate();
  delete drawer;
  delete parser;
  delete buffer;
  delete selectionFormat;
  delete searchFormat;
  delete HBar;
  delete VBar;
  delete timer;
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::Paint()
{
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::PaintWindow(HDC DC)
{
  RepaintWindow(false);
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::RepaintWindow(bool force)
{
  if(!_DEBUG_REPAINT || force)
  {
    NSpan * line = itrLine.line;
    for(int j = GetVisLineCount(), i = 0; i < j; i++)
    {
      parser->ParseFromLine(line, itrLine.linenum+i, 2);
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
  if(TSHEditFocused)
    return TSHEditFocused->ProcessKey(code, wParam, lParam);
  else return 0;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK TSHEdit::ProcessKey(int code, WPARAM wParam, LPARAM lParam)
{
  if(msgLock)
    return 0;


  msgLock = true;
  TShiftState shiftstate = KeyDataToShiftState(lParam);
  WORD Key = wParam;
  if((lParam & 2147483648) == 0)
  {
    if(wParam == 70)
      int a = 888;

    KeyDownHandler(this, Key, shiftstate);

    if(Key != 0)
    {
      wchar_t c;
      BYTE keyboard_state[256];

      GetKeyboardState(keyboard_state);
      if (ToUnicode(Key, (lParam & 0x0FF0000) >> 16, keyboard_state, &c, 1, 0) > 0)
      {
        KeyPressHandler(this, c);
        if(c != 0)
        {
          msgLock = false;
          return 0;
        }
      }
      else
      {
        msgLock = false;
        return 0;
      }
    }
  }
  else
  {
    KeyUpHandler(this, Key, shiftstate);
  }
  msgLock = false;
  return 1;
}
//---------------------------------------------------------------------------
void TSHEdit::Scroll(int by)
{
  Action("scrolling");
  if(by == 0 || buffer->GetLineCount() < GetVisLineCount()*3/4)
    return;
  int scrolled = 0;
  if(-by < GetVisLineCount() && by < GetVisLineCount())
  {
    while(by != 0)
    {
      if(by > 0)
      {
        if(itrLine.line->prevline)
        {
          itrLine.RevLine();
          scrolled++;
        }
        by--;
      }
      else
      {
        if(itrLine.line->nextline && itrLine.line->nextline->nextline && itrLine.line->nextline->nextline->nextline)
        {
          itrLine.GoLine();
          scrolled--;
        }
        by++;
      }
    }
    VBar->Position = itrLine.linenum;
    VBar->PageSize = GetVisLineCount();
    ProcessChange(0, scrolled, NULL);
    UpdateCursor(true);
  }

}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::KeyDownHandler(System::TObject * Sender,System::Word &Key, Classes::TShiftState Shift)
{
#ifndef _DEBUG_FORCE_SHORTCUTTS
  if(FOnKeyDown != NULL)
  {
    FOnKeyDown(this, Key, Shift);
  }
#endif
  if(itrCursor == itrCursorSecond && !Shift.Contains(ssShift))
    itrCursorSecond.Invalidate();

  Iter itrBeforeMove;
  if(Shift.Contains(ssShift))
  {
    if(!itrCursorSecond.Valid())
      itrCursorSecond = itrCursor;
    itrBeforeMove = itrCursor;
  }

  switch(Key)
  {
    case VK_DELETE:
    case VK_BACK:
      if (Shift.Contains(ssAlt))
      {
        UndoRedo(Shift.Contains(ssShift));
      }
      else if(!itrCursorSecond.Valid())
      {
        if(Key == VK_BACK && !(itrCursor.word->prev->prev || (itrCursor.word->prev && itrCursor.offset > 0)))  //have to stop after the head, not on it
        {
          break;
        }
        if(Key == VK_DELETE && !(itrCursor.word->next))  //have to stop after the head, not on it
        {
          break;
        }
        itrCursorSecond = itrCursor;
        if(Key == VK_BACK)
          itrCursor.RevChar();
        else
          itrCursorSecond.GoChar();
      }
      if(itrCursorSecond.Valid())
        DeleteSel();
      cursorLeftOffset = itrCursor.GetLeftOffset();
      AdjustLine(true);
      UpdateCursor(true);
      break;
#ifdef _DEBUG
    case VK_F2:
      LoadFile(L"test.txt");
      return;
    case VK_F3:
      parser->ParseFromLine(itrCursor.line, itrCursor.linenum, 2);
      parser->Execute();
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
    case VK_F7:
      {
        parser->dbgLogging = true;
        //parser->ParseFromLine(itrCursor->line, itrCursor->linenum, 2);
        //parser->Execute();
      }
      return;
    case VK_F8:
      {
        parser->dbgLogging = false;
      }
      return;
    case VK_F9:
      {
        itrCursor.GoLeft(3);
        UpdateCursor(true);
        RepaintWindow(true);
      }
      return;

    case VK_F10:
      {
        CheckIntegrity();
        CheckIterIntegrity(&itrCursor);
      }
      break;
#endif
    case VK_LEFT:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      if(Shift.Contains(ssCtrl))
        itrCursor.RevWordLiteral();
      else
        itrCursor.RevChar();
      AdjustLine(true);
      UpdateCursor(true);
      cursorLeftOffset = itrCursor.GetLeftOffset();
      Key = 0;
      break;
    case VK_RIGHT:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      if(Shift.Contains(ssCtrl))
        itrCursor.GoWordLiteral();
      else
        itrCursor.GoChar();
      AdjustLine(true);
      UpdateCursor(true);
      cursorLeftOffset = itrCursor.GetLeftOffset();
      Key = 0;
      break;
    case VK_UP:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      if(Shift.Contains(ssCtrl))
      {
        Scroll(1);
      }
      else
      {
        if(itrCursor.RevLine())
          itrCursor.GoByOffset(cursorLeftOffset);
        AdjustLine(true);
        UpdateCursor(true);
        Key = 0;
      }
      break;
    case VK_DOWN:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      if(Shift.Contains(ssCtrl))
      {
        Scroll(-1);
      }
      else
      {
        if(itrCursor.GoLine())
          itrCursor.GoByOffset(cursorLeftOffset);
        AdjustLine(true);
        UpdateCursor(true);
        Key = 0;
      }
      break;
    case VK_PRIOR:
    case VK_NEXT:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      cy += drawer->GetLinesize() * GetVisLineCount();
      my += drawer->GetLinesize() * GetVisLineCount();
      dy += drawer->GetLinesize() * GetVisLineCount();
      for(int i = GetVisLineCount(); i > 0; i--)
      {
        if(Key == VK_PRIOR)
          itrLine.RevLine();
        else
          itrLine.GoLine();
      }
      UpdateCursor(false);
      UpdateVBar();
      RepaintWindow(true);
      break;
    case VK_HOME:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      if(Shift.Contains(ssCtrl))
        itrCursor = buffer->begin();
      else
        itrCursor.GoLineStart();
      UpdateCursor(true);
      AdjustLine(true);
      break;
    case VK_END:
      if(!Shift.Contains(ssShift))
        ProcessMouseClear(true, true, false);
      if(Shift.Contains(ssCtrl))
        itrCursor = buffer->end();
      else
        itrCursor.GoLineEnd();
      UpdateCursor(true);
      AdjustLine(true);
      break;
      /* case 0x41:

         MSG msg = MSG();
         HANDLE handle = this->Handle;
         PeekMessage(&msg, this->Handle, WM_CHAR, WM_CHAR, 1);
         int a=555;
         break;   */
    default:
      return;
  }

  if(Shift.Contains(ssShift))
  {
    ParseScreenBetween(&itrCursor, &itrBeforeMove);
    ProcessNewSelection(true, false);
  }
  Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::KeyPressHandler(System::TObject * Sender, System::WideChar &Key)
{
  NSpan* changed = NULL;
  int linesMovedFrom = 0;

#ifndef _DEBUG_FORCE_SHORTCUTTS
  if(FOnKeyPress != NULL)
  {
    FOnKeyPress(this, Key);
  };
#endif
  switch(Key)
  {
    case 0x09: // Process a tab.
      {
        if(readonly)
          break;
        wchar_t * str = new wchar_t[2];
        str[0] = '\t';
        str[1] = '\0';
        AdjustLine(true);
        Insert(str);
        cursorLeftOffset = itrCursor.GetLeftOffset();
        delete str;
      }
      break;
      /*
         case 0x7F: // Process a delete
         case 0x08: // Process a backspace.

         break;
      //case 0x0A: // Process a linefeed.
      //    break;
      case 0x1B: // Process an escape.
      break;

      //case 0x0D: // Process a carriage return.
      //    break;
      case 0x6:
      {
      //TForm* searchBar = new TSearchBar(this);
      //searchBar->ShowModal() ;

      }
      break;      */
    case 0x03:
      Copy();
      break;
    case 0x18:
      if(readonly)
        break;
      Copy();
      DeleteSel();
      break;
    case 0x01:
      SelectAll();
      break;
      /*
    case 0x08: // Process a backspace.
      {
        if(readonly)
          break;
        bool alt = (0x8000 & GetKeyState(VK_MENU)); //MENU == ALT at microsoft
        if(alt)
        {
          bool shift = (0x8000 & GetKeyState(VK_SHIFT));
          UndoRedo(shift);
        }
      }
      break;     */
    case 0x1A: //
      {
        if(readonly)
          break;
        bool shift = (0x8000 & GetKeyState(VK_SHIFT));
        UndoRedo(shift);
      }
      break;
    case 0x16:
      if(readonly)
        break;
      Paste();
      break;
    case 0:
      break;
    default:
      if(readonly)
        break;
      if(Key < 0x20 && Key != '\r')
      {
        return;
      }
      changed = itrCursor.line;
      wchar_t * str = new wchar_t[2];
      str[0] = Key;
      str[1] = '\0';
      if(str[0] == '\r')
      {
        str[0] = '\n';
        linesMovedFrom = GetLineNum(changed)+1;
        if(itrLine.line->prevline == NULL)    //prevent line iter from being moved by insertion
          itrLine.pos = -1;
      }
      //linesMoved = buffer->Insert(itrCursor, str);
      AdjustLine(true);
      Insert(str);
      cursorLeftOffset = itrCursor.GetLeftOffset();
      break;
  }
  Key = 0;
}
//---------------------------------------------------------------------------
void TSHEdit::UndoRedo(bool redo)
{
  Iter * itr;
  Iter * itrbegin;
  if (redo)
  {
    Action("Redo");
    itr = buffer->Redo(itrbegin);
  }
  else
  {
    Action("Undo");
    itr = buffer->Undo(itrbegin);
  }
  if(itr != NULL && itr->Valid())
  {
    itrCursor = *itr;
    if(buffer->GetLineCount() < GetVisLineCount())
    {
      itrLine = buffer->begin();
    }
    AdjustLine(false);
    if(itrbegin != NULL && itrbegin->Valid())
    {
      parser->ParseFromLine(itrbegin->line, itrbegin->linenum, 0);
      delete itrbegin;
    }
    delete itr;
  }
  UpdateVBar();
  UpdateCursor(false);
  RepaintWindow(true);

  Action("  done");

#ifdef _DEBUG
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
//---------------------------------------------------------------------------
void __fastcall TSHEdit::KeyUpHandler(System::TObject * Sender,System::Word &Key, Classes::TShiftState Shift)
{

#ifndef _DEBUG_FORCE_SHORTCUTTS
  if(FOnKeyUp != NULL)
  {
    FOnKeyUp(this, Key, Shift);
  }
#endif
  switch (Key)
  {
    default:
      break;
  }
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::OnEnterHandler(TObject * Sender)
{
  int test =666;
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::WndProc(Messages::TMessage &Message)
{
  //bool processed = false;
  //NSpan* changed = NULL;
  //Iter * itr = NULL;
  //int linesMoved = 0;
  //int linesMovedFrom = 0;
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
          drawer->HMax = 100;
          drawer->UpdateLinenumWidth(buffer->GetLineCount());
          UpdateCursor(false);
          RepaintWindow(true);
          UpdateVBar();
          UpdateHBar();
        }
        else
        {
          scrolldelta += (short)HIWORD(Message.WParam);
          int scrolled = scrolldelta/120;
          scrolldelta = scrolldelta-scrolled*120;
          Scroll(scrolled*GetScrollStep());
        }
      }
      return;
    case WM_SETFOCUS:
      if(!KBHook)
        KBHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)ProcessKeyCall, NULL, GetCurrentThreadId());
      Parent->SetFocus();  //workaround - otherwise the parent form wont get focus
      this->SetFocus();
      TSHEditFocused = this;
      timer->Enabled = true;
      if(FOnEnter != NULL)
        FOnEnter(this);
      goto def;
    case WM_KILLFOCUS:
      if(FOnExit != NULL)
        FOnExit(this);
      UnhookWindowsHookEx(KBHook);
      KBHook = NULL;
      timer->Enabled = false;
      goto def;
    default:
def:
      this->TControl::WndProc(Message);
      return;
  }
}
//---------------------------------------------------------------------------
void TSHEdit::AdjustLine(bool paint)
{
  int l = GetLineNum(itrCursor.line);

  bool needsFullRepaint = false;
  bool needsHAdjustment = ((cx-HBar->Position) < drawer->GetLinenumWidth()+MARGINS && HBar->Position != 0) || (cx-HBar->Position) > this->Width - MARGINS;

  int movingby = 0;
  if(l >= 0 && l <= KEEP_VIS_TOP && itrLine.line->prevline != NULL)
  {
    for(int i = 0; i < KEEP_VIS_TOP - l; i++)
    {
      if(itrLine.line->prevline)
      {
        itrLine.RevLine();
        movingby--;
      }
    }
  }
  else if(l >= GetVisLineCount()-KEEP_VIS_BOTTOM)
  {
    for(int i = l; i > GetVisLineCount() - KEEP_VIS_BOTTOM; i--)
    {
      if(itrLine.line->nextline)
      {
        itrLine.GoLine();
        movingby++;
      }
    }
  }
  else if(l == -1)
  {
    if(buffer->GetLineCount() <= GetVisLineCount())
    {
      itrLine = buffer->begin();
    }
    else
    {
      itrLine = itrCursor;
      itrLine.GoLineStart();
      for(int i = 0; i < KEEP_VIS_TOP; i++)
        itrLine.RevLine();
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
      hpos = cx-drawer->GetLinenumWidth() - this->Width/2;

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
      Iter itr = itrLine;
      for(int i = movingby; i <= 0; i++)
      {
        parser->ParseFromLine(itr.line, itr.linenum, 2);
        itr.GoLine();
      }
      parser->Execute();
    }
    if(movingby > 0)
    {
      Iter itr = GetLineByNum(GetVisLineCount()-movingby);
      for(int i = movingby; i >= 0; i--)
      {
        parser->ParseFromLine(itr.line, itr.linenum, 2);
        itr.RevLine();
      }
      parser->Execute();
    }
  }
  else if (paint)
    parser->Execute();
}
//---------------------------------------------------------------------------
#ifdef _DEBUG
void TSHEdit::CheckIntegrity()
{
  int empty = 0;
  int eno = buffer->CheckIntegrity(empty);
  assert(empty == 0);
  assert(eno == 0);
  assert(buffer->GetItrCount() < 20);
}
//---------------------------------------------------------------------------
void TSHEdit::CheckIterIntegrity(Iter * itr)
{
  Span * sp = itr->word->prev;
  while(sp->string == NULL ||*(sp->string) != '\n')
    sp = sp->prev;
  assert((Span*)itr->line == sp);
  //("IterIntegrityCheck - iter on bad line");
  Iter * tmp = buffer->Begin();
  NSpan * np = itr->line;
  NSpan * dt = tmp->line;
  int j = 1;
  while(np->prevline != NULL)
  {
    assert(dt->nextline != NULL); //("IterIntegrityCheck - iter out of buffer - null in forward seek");
    j++;
    np = np->prevline;
    dt = dt->nextline;
  }
  assert(np == tmp->line);
  //("IterIntegrityCheck - backward seek fail");
  assert(dt == itr->line);
  //("IterIntegrityCheck - forward seek fail");
  assert(j == itr->linenum);
  //if(j != itr->linenum)
  //  int dbg=666;
  //("IterIntegrityCheck - wrong linenum");
  assert(itr->word->string <= itr->ptr && itr->ptr < itr->word->string + itr->word->length);
  delete tmp;
}
#endif _DEBUG
//---------------------------------------------------------------------------
void TSHEdit::ItrToXY(Iter * itr, int& x, int& y)
{
  int lnum = GetLineNum(itr->line);
  if(lnum >= 0)
  {
    x = X_OFF + drawer->GetLinenumWidth();
    y = Y_OFF + drawer->GetLinesize()*lnum;

    String str = buffer->GetLineTo(itr, true);
    if(!str.IsEmpty() && *(str.LastChar()) != '\0')           //null string is not an empty string... and null char gets drawn if it is only char in string
      x += this->Canvas->TextWidth(str);
  }
  y = (+itrCursor.linenum-itrLine.linenum)*drawer->GetLinesize();
}
//---------------------------------------------------------------------------
void TSHEdit::UpdateCursor(bool paint)
{
  ItrToXY(&itrCursor, cx, cy);
  drawer->UpdateCursor(cx,cy);
  if(paint)
  {
    drawer->Paint();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::MouseDown(TMouseButton Button, Classes::TShiftState Shift, int X, int Y)
{
  this->SetFocus();
  Action("Placing cursor", false);

  if(FOnClick != NULL)
    FOnClick(this);

  dx = X;
  dy = Y;

  ProcessMouseClear(true, true);
  itrCursor = XYtoItr(X, Y);
  cursorLeftOffset = itrCursor.GetLeftOffset();

  cx = X;
  cy = Y;

  mx = cx;
  my = cy;

  UpdateCursor(true);
  //drawer->UpdateCursor(cx,cy);
  //drawer->Paint();
  //parser->Execute();

  mouseDown = true;
  Action("    Cursor placed");
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::MouseMove(Classes::TShiftState Shift, int X, int Y)
{
  if(!mouseDown && !mouseDoubleClickFlag)
    return;
  if(mouseDoubleClickFlag && (abs(X-dx) > 2 || abs(Y-dy) > 2))
    mouseDoubleClickFlag = false;
  if(!mouseDown)
    return;
  if(!mouseSelect && (abs(X-dx) > 2 || abs(Y-dy) > 2))
    mouseSelect = true;
  if(!mouseSelect)
    return;

#ifndef _DEBUG_CURSOR
  ProcessMouseMove(X, Y);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::MouseUp(TMouseButton Button, Classes::TShiftState Shift, int X, int Y)
{
  if(FOnMouseUp != NULL)
    FOnMouseUp(this, Button, Shift, X, Y);

  if(mouseDoubleClickFlag)
  {
    ProcessMouseClear(true, false);
    itrCursor = XYtoItr(X, Y);
    itrCursorSecond = itrCursor;
    itrCursor.GoWordEndLiteral();
    itrCursorSecond.RevWordLiteral();
    ProcessNewSelection();
  }
  else if(mouseSelect)
  {
    Action("    Selecting ...", false);
    ProcessMouseMove(X, Y);
    Action("    Selected " + Escape(SelText));
  }

  mouseDown = false;
  mouseSelect = false;
  mouseDoubleClickFlag = !mouseDoubleClickFlag;
}
//---------------------------------------------------------------------------
void TSHEdit::ProcessMouseMove(int &x, int &y)
{
  if(!itrCursorSecond.Valid())
    itrCursorSecond = itrCursor;

  Iter oldIter = itrCursor;
  Iter tmp  = XYtoItr(x, y);
  if(tmp == itrCursor)
  {
    return;
  }
  ProcessMouseClear(false, false);
  itrCursor = tmp;

  if(oldIter.Valid())
    ParseScreenBetween(&oldIter, &itrCursor);

  ProcessNewSelection(false, false);

  mx = x;
  my = y;
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
void TSHEdit::ProcessNewSelection(bool execredraw, bool draw)
{
  //ProcessMouseClear(false, false, false);
  selectionFormat->RemoveAllMarks();
  if(itrCursor == itrCursorSecond)
    itrCursorSecond.Invalidate();
  if(!itrCursorSecond.Valid())
  {
    UpdateCursor(execredraw);
    return;
  }
  UpdateCursor(false);
  if(itrCursor != itrCursorSecond)
  {
    GetCursorIter()->IMarkupBegin(selectionFormat);
    GetCursorIterEnd()->IMarkupEnd(selectionFormat);
  }

  if(draw)
    ParseScreenBetween(&itrCursor, &itrCursorSecond);

  if(execredraw)
    parser->Execute();
}
//---------------------------------------------------------------------------
void TSHEdit::SetSelection(Iter * first, Iter * second)
{
  Action("Setting new selection");
  selectionFormat->RemoveAllMarks();
  ParseScreenBetween(GetCursorIter(), first);
  ParseScreenBetween(GetCursorIterEnd(), second);
  //if(first->buffer == NULL || second->buffer == NULL)
  //  std::cout << "positionless iterators passed to set selection" << std::endl;
  itrCursor = *first;
  itrCursorSecond = *second;
  AdjustLine(true);
  ProcessNewSelection();
}
//---------------------------------------------------------------------------
void TSHEdit::ProcessMouseClear(bool redraw, bool deleteiter, bool execredraw)
{
  selectionFormat->RemoveAllMarks();

  if(itrCursorSecond.Valid())
  {
    if(itrCursor != itrCursorSecond && redraw)
    {
      ParseScreenBetween(GetCursorIter(), GetCursorIterEnd());
      if(execredraw)
        parser->Execute();
    }
    if(deleteiter)
      itrCursorSecond.Invalidate();
  }
}
//---------------------------------------------------------------------------
Iter * TSHEdit::GetCursorIter()
{
  return (itrCursorSecond.Valid() && itrCursor > itrCursorSecond) ? &itrCursorSecond : &itrCursor;
}
//---------------------------------------------------------------------------
Iter * TSHEdit::GetCursorIterEnd()
{
  return (itrCursorSecond.Valid() && itrCursor < itrCursorSecond) ? &itrCursorSecond : &itrCursor;
}
//---------------------------------------------------------------------------

Iter TSHEdit::XYtoItr(int& x, int& y)
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

  Iter itr = GetLineByNum(y/drawer->GetLinesize(), false);
  int xSum = X_OFF-HBar->Position+drawer->GetLinenumWidth();
  y = (itr.line->nextline->next) ? drawer->GetLinesize()*(y/drawer->GetLinesize()) : drawer->GetLinesize()*GetLineNum(itr.line); //to normalize cursor position; to prevent problems with end of file; and to optimize both
  String line = buffer->GetLine(&itr, true);
  int w = Canvas->TextWidth(line);

  if(w+xSum < x || w == 0)
  {
    x = xSum+w;
    itr.GoLineEnd();
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
    itr.GoByOffset(offset);
    return itr;
  }
}
//---------------------------------------------------------------------------
Iter __fastcall TSHEdit::GetLineByNum(int no, bool allowEnd)
{
  Iter itr = itrLine;
  for(int i = 0; i < no; i++)
    if(!itr.GoLine(allowEnd))
      break;
  return itr;
}
//---------------------------------------------------------------------------
//nope, default value does not work... it passes the value EVERY time, no matter whether specified or not
Iter __fastcall TSHEdit::GetLineByNum(int no)
{
  return GetLineByNum(no, false);
}
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetVisLineCount()
{
  return this->Height/drawer->GetLinesize()+1;
}
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetLineCount()
{
  return buffer->GetLineCount();
}
//---------------------------------------------------------------------------
#ifdef _DEBUG
void __fastcall TSHEdit::dbgIter()
{
  Log("cursor pos at "+String(itrCursor.offset)+":"+itrCursor.word->string+" at line "+itrCursor.line->next->string+"; prevline "+(itrCursor.line->prevline != NULL ? itrCursor.line->prevline->next->string : L"")+"+ nextline "+(itrCursor.line->nextline != NULL ? itrCursor.line->nextline->string : L""));
}
#endif
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetLineNum(NSpan * line)
{
  NSpan * testline = itrLine.line;
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
void TSHEdit::DeleteSel(bool allowsync, Iter * start, Iter * end)
{
  if(start == NULL)
    start = GetCursorIter();
  if(end == NULL)
    end = GetCursorIterEnd();
  Action("Deleting selection '" + Escape(SelText) + "' ...", false);
  int linesMovedFrom = (end->linenum-itrLine.linenum)+1;

  int linesMoved = -buffer->Delete(start, end);

  bool needRepaint = start->linenum < itrLine.linenum;

  NSpan* changed = itrCursor.line;
  selectionFormat->RemoveAllMarks();
  itrCursorSecond.Invalidate();

  if(linesMoved != 0)
    needRepaint |= drawer->UpdateLinenumWidth(buffer->GetLineCount());

  if(needRepaint) //means that line iterator has been moved as well
  {
    itrLine = itrCursor;
    AdjustLine(false);
    RepaintWindow(true);
  }
  else
  {
    ProcessChange(linesMovedFrom < 0 ? 0 : linesMovedFrom, linesMoved, changed);
  }
#ifdef _DEBUG_UNDO
  UndoCheck();
#endif
  if(FOnChange != NULL)
    FOnChange(this);
  Action("    done");
}
//---------------------------------------------------------------------------
void TSHEdit::Insert(const wchar_t * text, Iter * itr)
{
  Action(String("Inserting text '") + Escape(text) + L"' ... ", false);
  bool atStart = itrLine.linenum == 1 && itrLine.pos == 0 && itrCursor == itrLine;
  bool changeSent = false;

  if(itr == NULL)
  {
    if(itrCursorSecond == itrCursor)
      itrCursorSecond.Invalidate();
    if(itrCursorSecond.Valid())
    {
      changeSent = true;
      DeleteSel(false);
    }
    itr = &itrCursor;
  }


  parser->ParseFromLine(itr->line, itr->linenum, 0);

  NSpan* changed = itr->line;
  int linesMovedFrom = GetLineNum(itr->line)+1;
  int linesMoved = buffer->Insert(itr, text);
  itrLine.GoLineStart();
#ifdef FIXSCREENTOP
  if(atStart)
  {
    itrLine = buffer->begin();
  }
#endif
  //itrCursor->linenum += linesMoved;
  UpdateCursor(false);

  if(linesMoved != 0 && drawer->UpdateLinenumWidth(buffer->GetLineCount()))
  {
    UpdateVBar();
    RepaintWindow(true);
  }
  else
    ProcessChange(linesMovedFrom, linesMoved, changed);

#ifdef _DEBUG_UNDO
  UndoCheck();
#endif

  if(FOnChange != NULL && !changeSent)
    FOnChange(this);
  Action("    done");
}
//---------------------------------------------------------------------------
void TSHEdit::ProcessChange(int linesMovedFrom, int linesMoved, NSpan * changed)
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
      int barcorrection = HBar->Visible ? HBar->Height/drawer->GetLinesize()+2 : 2;
      Iter it = GetLineByNum(GetVisLineCount()+linesMoved-barcorrection);
      for(int i = linesMoved-barcorrection; i < 0 && it.line->nextline; i++, it.GoLine())
        parser->ParseFromLine(it.line, it.linenum, 2);
    }
    if(linesMoved > 0)
    {
      //scroll up (move image down)
      Iter it = itrLine;
      for(int i = 0; i < linesMoved && it.line->nextline && i < GetVisLineCount(); i++, it.GoLine())
        parser->ParseFromLine(it.line, it.linenum,2);
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
    parser->ParseFromLine(changed, itrLine.linenum+linenum, linenum >= 0 ? 2 : 1);
    parser->ParseFromLine(itrCursor.line, itrCursor.linenum, linenum >= 0 ? 2 : 1);
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
void TSHEdit::UpdateVBar()
{
  if(buffer->GetLineCount() >= GetVisLineCount() - 5 && buffer->GetLineCount() >= GetVisLineCount()*3/4)
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
    VBar->Position = itrLine.linenum;
  }
  else if(VBar->Visible)
  {
    VBar->Hide();
    drawer->DrawResize(this->Width, this->Height);
    itrLine = buffer->begin();
    RepaintWindow(true);
    return;
  }
}
//---------------------------------------------------------------------------
void TSHEdit::UpdateHBar()
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
void TSHEdit::Copy()
{
  if(itrCursorSecond == itrCursor)
    itrCursorSecond.Invalidate();
  if(itrCursorSecond.Valid())
  {
    wchar_t * str = buffer->GetText(GetCursorIter(), GetCursorIterEnd());
    clipboard->SetTextBuf(str);
    delete[] str;
  }
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::OnVScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos)
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

  if(abs(ScrollPos-itrLine.linenum) < GetVisLineCount())
  {
    Scroll(itrLine.linenum-ScrollPos);
  }
  else
  {

    cy += drawer->GetLinesize() * (itrLine.linenum - ScrollPos);
    my += drawer->GetLinesize() * (itrLine.linenum - ScrollPos);
    dy += drawer->GetLinesize() * (itrLine.linenum - ScrollPos);
    if(ScrollPos < itrLine.linenum)
    {
      if(ScrollPos > itrLine.linenum/2 && ScrollPos > 100)
        itrLine.GoToLine(ScrollPos);
      else
      {
        itrLine = buffer->begin();
        itrLine.GoToLine(ScrollPos);
      }
    }
    else
    {
      itrLine.GoToLine(ScrollPos);
    }
    ScrollPos = -1;
    drawer->UpdateCursor(cx, cy);
    RepaintWindow(true);
  }

  if(ScrollCode == scEndScroll)
    this->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::OnHScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos)
{
  if(ScrollCode == scEndScroll)
  {
    RepaintWindow(true);
    this->SetFocus();
  }
}
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetActLine()
{
  return itrLine.linenum;
}
//---------------------------------------------------------------------------
void TSHEdit::Paste()
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
  delete [] buf;
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::OnResizeCallback(TObject * Sender)
{
  UpdateHBar();
  UpdateVBar();
  drawer->DrawResize(this->Width, this->Height);

}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::OnTimer(TObject * Sender)
{
  drawer->Paint();
}
//---------------------------------------------------------------------------
bool __fastcall TSHEdit::IsLineFirstVisible(NSpan * line)
{
  return line == itrLine.line;
}
//---------------------------------------------------------------------------
void TSHEdit::SelectAll()
{
  this->ProcessMouseClear(false, true);
  itrCursor = buffer->begin();
  itrCursorSecond = buffer->end();
  if(itrCursor != itrCursorSecond)
  {
    itrCursor.IMarkupBegin(this->selectionFormat);
    itrCursorSecond.IMarkupEnd(this->selectionFormat);
  }
  parser->ParseFromToLine(itrLine.line, itrLine.linenum, GetVisLineCount(), 2);
  UpdateCursor(false);
  parser->Execute();
}
//---------------------------------------------------------------------------
void TSHEdit::LoadFile(const wchar_t * filename)
{
  buffer->SimpleLoadFile(filename);
  itrCursor = buffer->begin();
  itrLine = buffer->begin();
  parser->InvalidateAll();
  parser->ParseFromLine(itrLine.line, 1, 0);
  UpdateCursor(false);
  UpdateVBar();
  RepaintWindow(true);
}
//---------------------------------------------------------------------------
void TSHEdit::SaveFile(const wchar_t * filename)
{
  buffer->SimpleSaveFile(filename);
}
//---------------------------------------------------------------------------
void TSHEdit::ParseScreenBetween(Iter * it1, Iter * it2)
{
  if(it1 == NULL || it2 == NULL || !it1->Valid() || !it2->Valid())
  {
    if(it1 != NULL || ! it1->Valid())
      parser->ParseFromLine(it1->line, it1->linenum, 2);
    if(it2 != NULL || !it2->Valid())
      parser->ParseFromLine(it2->line, it2->linenum, 2);
    return;
  }
  if(it2->linenum < it1->linenum || (it2->linenum == it1->linenum && it2->pos < it1->pos))
  {
    Iter * tmp = it1;
    it1 = it2;
    it2 = tmp;
  }
  if(it2->linenum < itrLine.linenum)
    return;

  if(it1->linenum < itrLine.linenum)
    it1 = &itrLine;

  int count = it2->linenum - it1->linenum+1;
  if(count > GetVisLineCount())
    count = GetVisLineCount();

  parser->ParseFromToLine(it1->line, it1->linenum, count, 2);
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::SetFontsize(int size)
{
  drawer->SetFontsize(size);
}
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetFontsize()
{
  return drawer->GetFontsize();
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::SetLinenumsEnabled(bool enabled)
{
  drawer->SetLinenumsEnabled(enabled);
}
//---------------------------------------------------------------------------
bool __fastcall TSHEdit::GetLinenumsEnabled()
{
  return drawer->GetLinenumsEnabled();
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::SetLanguageDefinition(LanguageDefinition * def)
{
  parser->SetLangDef(def);
}
//---------------------------------------------------------------------------
void TSHEdit::Action(String msg, bool end)
{
#ifdef _DEBUG_LOGGING
  Write(msg + PositionDescription(), end);
#endif
#ifdef _DEBUG
  CheckIntegrity();
  CheckIterIntegrity(&itrLine);
  CheckIterIntegrity(&itrCursor);
#endif
}
//---------------------------------------------------------------------------
String TSHEdit::Escape(String str)
{
  for(int i = 1; i <= str.Length(); i++)
    if( str[i] < 32)
      str[i] = '_';
  return str;
}
//---------------------------------------------------------------------------
#ifdef _DEBUG
void __fastcall TSHEdit::Log(String str)
{
  if(dbgLog != NULL)
    this->dbgLog->Lines->Add(str);
}
//---------------------------------------------------------------------------
void TSHEdit::UndoCheck()
{
  Action("UndoCheck", false);
  int x = itrCursor.GetLeftOffset();
  int y = itrCursor.linenum;
  Iter * a;
  Iter * b;
  a = buffer->Undo(b);
  delete a;
  delete b;
  Action("RedoCheck", false);
  a = buffer->Redo(b);
  delete a;
  delete b;
  Action("done", false);
  itrCursor.GoToLine(y) ;
  itrCursor.GoByOffset(x);
}
//---------------------------------------------------------------------------
String TSHEdit::PositionDescription()
{
  return String(" --- ") + GetCursorCaretY() + ":" + GetCursorCaretX() + " '" + Escape(SelText) + "'";
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TSHEdit::Write(String message, bool end)
{
#ifdef _DEBUG_LOGGING
  assert(myfile.is_open());
  myfile << UTF8String(message.c_str()).c_str();
  if(end)
    myfile << std::endl;
#endif
}
#endif
//---------------------------------------------------------------------------
void __fastcall TSHEdit::SetSelLen(int SelLen)
{
  if(itrCursorSecond.Valid())
  {
    ProcessMouseClear(false, true);
  }
  Iter itr = itrCursor;
  itr.GoBy(SelLen, true);
  SetSelection(&itrCursor, &itr);
  //ProcessNewSelection(); //already done in setselection
}
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetSelLen()
{
  if(!itrCursorSecond.Valid())
    return 0;
  else
    return GetCursorIter()->GetDistance(GetCursorIterEnd());
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::SetSelText(String SelText)
{
  Insert(SelText.c_str());
}
//---------------------------------------------------------------------------
String __fastcall TSHEdit::GetSelText()
{
  if(!GetCursorIter()->Valid() || !GetCursorIterEnd()->Valid() == NULL)
    return L"";
  {
    wchar_t* str = buffer->GetText(GetCursorIter(), GetCursorIterEnd());
    String rtn = String(str);
    delete [] str;
    return rtn;
  }
}
//---------------------------------------------------------------------------
String TSHEdit::GetRange(Iter * begin, Iter * end)
{
  if(*begin < *end)
  {
    wchar_t* str = buffer->GetText(begin, end);
    String rtn = String(str);
    delete [] str;
    return rtn;
  }
  else if (*begin > *end)
  {
    wchar_t * str = buffer->GetText(begin, end);
    String rtn = String(str);
    delete [] str;
    return rtn;
  }
  else
    return L"";
}
//---------------------------------------------------------------------------
CIter TSHEdit::begin()
{
  return CIter(this, buffer->begin(), false, true);
}
//---------------------------------------------------------------------------
CIter TSHEdit::end()
{
  return CIter(this, buffer->end(), false, true);
}
//---------------------------------------------------------------------------
CIter TSHEdit::GetCursor()
{
  return CIter(this, *GetCursorIter(), false, false);
}
//---------------------------------------------------------------------------
CIter TSHEdit::GetCursorEnd()
{
  return CIter(this, *GetCursorIterEnd(), true, false);
}
//---------------------------------------------------------------------------
int TSHEdit::GetCursorX()
{
  return cx;
}
//---------------------------------------------------------------------------
int TSHEdit::GetCursorY()
{
  return cy;
}
//---------------------------------------------------------------------------
int TSHEdit::GetCursorCaretX()
{
  return GetCursorIter()->linenum;
}
//---------------------------------------------------------------------------
int TSHEdit::GetCursorCaretY()
{
  return GetCursorIter()->pos;
}
//---------------------------------------------------------------------------
int __fastcall TSHEdit::GetLineHeight()
{
  return drawer->linesize;
}
//---------------------------------------------------------------------------
void TSHEdit::Clear()
{
  Action("Clearing");
  Iter * beg = buffer->Begin();
  Iter * end = buffer->End();
  buffer->Delete(beg,end);
  delete beg;
  delete end;
  RepaintWindow(true);
}
//---------------------------------------------------------------------------
void TSHEdit::AddLine(const String& string, SHEdit::Format * format)
{

  Action("Adding line");
  Iter end = buffer->end();
  bool begin = end == itrLine && itrLine.linenum == 1;
  parser->ParseFromLine(end.line, end.linenum, Visible(&end) ? 2 : 0);
  buffer->Insert(&end, string.c_str());
  buffer->Insert(&end, L"\n");
  if(begin)
    itrLine = buffer->begin();
  if(format != NULL)
  {
    end.RevChar();
    end.MarkupEnd(format);
    end.GoLineStart();
    end.MarkupBegin(format);
  }

  UpdateCursor(false); //mohl byt posunut insertem
  parser->Execute();
}
//---------------------------------------------------------------------------
void TSHEdit::AddLines(const String& string, SHEdit::Format * format)
{
  AddLine(string);
}
//---------------------------------------------------------------------------
void __fastcall TSHEdit::SetText(String str)
{
  Clear();
  Insert(str.c_str());
}
//---------------------------------------------------------------------------
String __fastcall TSHEdit::GetText()
{
  String retString = L"";
  Iter * itr = buffer->Begin();
  do
    retString += itr->GetLine() + L"\n";
  while ( itr->GoLine(false));
  return retString;
}
//---------------------------------------------------------------------------
bool TSHEdit::Visible(Iter * itr)
{
  return itrLine.linenum <= itr->linenum && itr->linenum <= itrLine.linenum+GetVisLineCount();
}
//---------------------------------------------------------------------------
void TSHEdit::MarkAll(const String& text, bool caseSensitive, bool wholeWord)
{
  Iter * itr = buffer->Begin();
  while(itr->FindNext(text.c_str(), false, caseSensitive, wholeWord))
  {
    itr->MarkupBegin(searchFormat);
    itr->GoBy(text.Length());
    itr->MarkupEnd(searchFormat);
  }
  RepaintWindow();
}
//---------------------------------------------------------------------------
void TSHEdit::UnMarkMarked()
{
  searchFormat->RemoveAllMarks();
  RepaintWindow();
}
//---------------------------------------------------------------------------
String TSHEdit::GetLine(Iter* itr)
{
  return buffer->GetLine(itr, false);
}
//---------------------------------------------------------------------------
String TSHEdit::GetLine(int index)
{
  Iter * itr = buffer->Begin();
  itr->GoToLine(index);
  String rtnStr = itr->GetLine();
  delete itr;
  return rtnStr;
}
//---------------------------------------------------------------------------
int TSHEdit::GetScrollStep()
{
  return maxScrollStep > GetVisLineCount() ? GetVisLineCount()/2 : maxScrollStep;
}
//---------------------------------------------------------------------------
void _fastcall TSHEdit::SetKeepHistory(bool keep)
{
  buffer->keepHistory = keep;
  buffer->HistoryOnOff();
}
//---------------------------------------------------------------------------
bool _fastcall TSHEdit::GetKeepHistory()
{
  return buffer->keepHistory;
}
//---------------------------------------------------------------------------
LanguageDefinition* __fastcall TSHEdit::GetLanguageDefinition()
{
  return parser->GetLangDef();
}
//---------------------------------------------------------------------------
