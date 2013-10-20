
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
  enum DrawType {Text, Move, Cursor, Endline, Eof};
  //---------------------------------------------------------------------------
  struct DrawTask
  {
    DrawType type;
  };
  //---------------------------------------------------------------------------

  struct DrawTaskEndline : public DrawTask
  {
    short linenum;
    DrawTaskEndline();
  };
  //---------------------------------------------------------------------------

  struct DrawTaskText : public DrawTask
  {
    DrawTaskText();
    ~DrawTaskText();
    bool newline;
    short linenum;
    String * text;
    FontStyle format;
  };
  //---------------------------------------------------------------------------

  struct DrawTaskEof : public DrawTask
  {
    short linenum;
    DrawTaskEof();
  };
  //---------------------------------------------------------------------------

  struct DrawTaskCursor : public DrawTask
  {
    int x;
    int y;
    DrawTaskCursor();
    DrawTaskCursor(int x, int y);
  };
  //---------------------------------------------------------------------------

  struct DrawTaskMove : public DrawTask
  {
    DrawTaskMove(short from, short to, short by);
    DrawTaskMove();
    short from;
    short to;
    short by;
  };
  //---------------------------------------------------------------------------
  class Drawer : public TThread
  {
    private:
      TSQLEdit * parent;
      std::list<DrawTask*> tasklist;

      HANDLE drawerQueueMutex;
      HANDLE drawerCanvasMutex;
      HANDLE drawerTaskPending;

      TCanvas * canvas;
      TCanvas * drawcanvas;
      Graphics::TBitmap * bitmap;

      int x, y;
      int cx, cy;
      bool con;

      void DrawCursor();
#ifdef DEBUG
      void Write(AnsiString message);
      void QueueDump();
#endif
    public:
      virtual void __fastcall Execute(void); 

      __fastcall Drawer(TCanvas * Canvas, TSQLEdit * parent, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending) ;
      virtual __fastcall ~Drawer();

      void Draw(DrawTask* drawtask);
  };
}
//---------------------------------------------------------------------------
#endif