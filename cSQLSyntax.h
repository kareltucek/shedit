//---------------------------------------------------------------------------

#ifndef cSQLSyntaxH
#define cSQLSyntaxH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <Controls.hpp>
#include <Clipbrd.hpp>
#include <time.h>
#include "uIter.h"
#include "uBuffer.h"
#include "uDrawer.h"
#include "uParser.h"
//---------------------------------------------------------------------------
/*
 * Keyboard is trapped at 2 places - first is normal overloaded mwinproc essage for text input
 *     the second is a windows hook which lets us intercept all keys we want, before they're
 *     passed to the program (that's neccessary to intercept input that would be otherwise
 *     consumed by vcl itself and would not be sent to component at all
 *
 * 
 */
//---------------------------------------------------------------------------


#include "config"

namespace SHEdit
{
  LRESULT CALLBACK ProcessKeyCall(int code, WPARAM wParam, LPARAM lParam);

  class PACKAGE TSQLEdit : public TCustomControl
  {
    private:
      Iter * itrLine;
      Iter * itrCursor;
      Iter * itrCursorSecond;
      Buffer * buffer;

      HHOOK KBHook;

      bool recmsg;  //used to synchronize message processing - all messages should be processed before passing job to parser


      HANDLE bufferChanged;
      HANDLE bufferMutex;
      HANDLE drawerQueueMutex;
      HANDLE drawerCanvasMutex;
      HANDLE drawerTaskPending;

      Parser * parser;
      Drawer * drawer;

      TScrollBar * HBar;
      TScrollBar * VBar;
      void UpdateVBar();
      void UpdateHBar();
      clock_t lastHBarUpdate;

      Iter * XYtoItr(int& x, int& y);
      void UpdateCursor();
      void ProcessMouseMove(int& x, int& y);
      void ProcessMouseClear(bool redraw, bool deletecursord);
      Format * selectionFormat;
      bool mouseDown;
      bool mouseSelect;
      bool cursorsInInvOrder;
      int dx, dy; //down
      int cx, cy; //cursor
      int mx, my; //mouse move  pos
      int cursorLeftOffset;
      int scrolldelta; //used by ms to treat smooth-scroll wheels...

      void AdjustLine();
      void Scroll(int by);
      void __fastcall OnVScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos);
      void __fastcall OnHScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos);
      void __fastcall OnResizeCallback(TObject* Sender);

    protected:
      virtual void __fastcall Paint();
      virtual void __fastcall PaintWindow(HDC DC);
      virtual void __fastcall RepaintWindow(bool force);

      virtual void __fastcall WndProc(Messages::TMessage &Message);

      DYNAMIC void __fastcall MouseDown(TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
      DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
      DYNAMIC void __fastcall MouseUp(TMouseButton Button, Classes::TShiftState Shift, int X, int Y);

      TClipboard * clipboard;
      void Copy();
      void Paste();
      void DeleteSel(bool allowsync = true);
      void Insert(wchar_t * text);
      void ProcessChange(int linesMovedFrom, int linesMoved, NSpan * changed);

    public:
#ifdef DEBUG
      void __fastcall dbgIter();
      void __fastcall Log(String str);
      void Write(AnsiString message);
#endif
      friend class Drawer;

      TMemo * dbgLog;
      int __fastcall GetVisLineCount();
      int __fastcall GetLineNum(NSpan * line);
      bool __fastcall GetLineFirst(NSpan * line);
      Iter * __fastcall GetLineByNum(int num, bool allowEnd);
      Iter * __fastcall GetLineByNum(int num);

      Iter * GetCursor();
      Iter * GetCursorEnd();

      void SelectAll();
      void LoadFile(wchar_t * filename);

      virtual LRESULT CALLBACK ProcessKey(int code, WPARAM wParam, LPARAM lParam);

      __fastcall TSQLEdit(TComponent* Owner);
      __fastcall ~TSQLEdit();

__published:
      __property TAlign Align = {read=FAlign, write=SetAlign, default=0};
  };
  //---------------------------------------------------------------------------
}
#endif
