
//---------------------------------------------------------------------------

#ifndef uDrawerH
#define uDrawerH
#include <windows.h>
#include <vcl.h>
#include <list>
#include "config.h"
#include "uFormat.h"

namespace SHEdit
{
  class NSpan;
  class TSQLEdit;
  class FontStyle;

  /*!
   * Drawer ensures drawing everything that it is told to draw. It is usually told by TSQLEdit to move parts of already-painted information upwards or downwards, to update/repaint cursor and to update it's fontsize or linenumwidth parameters, while by Parser it is ordered to draw text, endlines and eof.
   *
   * When drawing text, the Drawer::x is incremented with width of drawn text waiting for endline to bleach the rest of screen and reset the x again to 0. Also information about length of longest line is kept for setting of horizontal scrollbar's parameters. Just highest 'ever reached' width is kept - in current structure there's no way to keep this parameter up to date (and it's just a visual glitch).
   *
   * Here is not much to document either, just to mention that everything that is not apparently pixel position is usually screen-line position (that means i.e. nth visible line).
   *
   * One more note - UpdateFontSize and updateLinenumWidth does not automatically repaint screen - screen repaint has to be triggered manually.
   * */
  //---------------------------------------------------------------------------
  class Drawer
  {
    private:
      TSQLEdit * parent;

      TCanvas * canvas;
      TCanvas * drawcanvas;

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
      int lastlinenumcount; //to be able to update lw automatically

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

      __fastcall Drawer(TCanvas * Canvas, TSQLEdit * parent) ;
      virtual __fastcall ~Drawer();

      void DrawCursor();
      void __fastcall DrawText(String text, bool newline, short linenum, FontStyle format);
      void __fastcall DrawMove(int from, int to, int by);
      void __fastcall DrawEof(short linenum);
      void __fastcall UpdateCursor(int x, int y);
      void __fastcall DrawResize(int w, int h);
      void __fastcall DrawEndl(short linenum, FontStyle format);
      void __fastcall DrawLinenum(int from);
      void __fastcall Paint();

      void __fastcall SetFontsize(int size);
      bool __fastcall UpdateLinenumWidth(int count);
      int __fastcall GetLinenumWidth();
      int __fastcall GetFontsize();
      int __fastcall GetLinesize();
      void __fastcall SetLinenumsEnabled(bool enable);
      bool __fastcall GetLinenumsEnabled();

    public:
      friend class TSQLEdit;
  };
}
//---------------------------------------------------------------------------
#endif
