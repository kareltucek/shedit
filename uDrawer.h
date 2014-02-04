
//---------------------------------------------------------------------------

#ifndef uDrawerH
#define uDrawerH
#include <windows.h>
#include <vcl.h>
#include <list>
#include "config"
#include "uFormat.h"

namespace SHEdit
{
  class NSpan;
  class TSQLEdit;
  class FontStyle;
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  class Drawer
  {
    private:
      TSQLEdit * parent;

      HANDLE drawerQueueMutex;
      HANDLE drawerCanvasMutex;
      HANDLE drawerTaskPending;

      TCanvas * canvas;
      TCanvas * drawcanvas;

      //double buffer debug test
      Graphics::TBitmap * bitmap;

      inline int RightBorder();
      inline int BottomBorder();

      int x, y;
      int cx, cy;
      bool con;   //cursorOn
      TColor cursorBGcolor;

      int fontsize;
      int linesize;
      int linenumwidth;

      bool linenumsenabled;

      int debugtasks;
      int debugcount;

      int HPos, HMax;
      void __fastcall UpdateHBar();

#ifdef DEBUG
      void Write(AnsiString message);
      void QueueDump();
#endif
    public:

      __fastcall Drawer(TCanvas * Canvas, TSQLEdit * parent, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending) ;
      virtual __fastcall ~Drawer();

            void DrawCursor();
void __fastcall DrawText(String text, bool newline, short linenum, FontStyle format);
void __fastcall DrawMove(int from, int to, int by);
void __fastcall DrawEof(short linenum);
void __fastcall UpdateCursor(int x, int y);
void __fastcall DrawResize(int w, int h);
void __fastcall DrawEndl(short linenum);
void __fastcall DrawLinenum(int from);
void __fastcall Paint();

void __fastcall SetFontsize(int size);
bool __fastcall UpdateLinenumWidth(int count);
int __fastcall GetLinenumWidth();
int __fastcall GetFontsize();
int __fastcall GetLinesize();
void __fastcall SetLinenumsEnabled(bool enable);

    public:
      friend class TSQLEdit;
  };
}
//---------------------------------------------------------------------------
#endif
